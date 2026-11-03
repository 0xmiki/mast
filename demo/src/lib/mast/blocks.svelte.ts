/**
 * The whole product in one store: sites, grouped by category, and how long each one is blocked.
 * A block can be lengthened, never shortened. "Forever" really is forever.
 * The only way back is a 10-second undo for mis-clicks, right after blocking.
 */
import { clock } from './clock.svelte';
import { PRESETS, isException } from './blocklist';
import { DAY, HOUR, UNDO_MS, startOfDay } from './engine';
import type { Attempt } from './types';
import { hostOf, isInternal, matches } from './url';

const KEY = 'mast.v3';

/** End time in ms, or null for forever. */
export type Until = number | null;

export interface Block {
	since: number;
	until: Until;
}

export const DURATIONS = [
	{ id: '1h', short: '1h', label: 'for 1 hour' },
	{ id: '4h', short: '4h', label: 'for 4 hours' },
	{ id: 'today', short: 'today', label: 'until midnight' },
	{ id: 'week', short: '1 wk', label: 'for a week' },
	{ id: 'month', short: '1 mo', label: 'for a month' },
	{ id: 'forever', short: '∞', label: 'forever' }
] as const;
export type DurationId = (typeof DURATIONS)[number]['id'];

export function untilFor(id: DurationId, now: number): Until {
	switch (id) {
		case '1h':
			return now + HOUR;
		case '4h':
			return now + 4 * HOUR;
		case 'today':
			return startOfDay(now) + DAY;
		case 'week':
			return now + 7 * DAY;
		case 'month':
			return now + 30 * DAY;
		case 'forever':
			return null;
	}
}

/** Is `a` at least as long as `b`? Forever beats everything. */
export function outlasts(a: Until, b: Until) {
	return a === null || (b !== null && a >= b);
}

export interface Category {
	id: string;
	name: string;
	sites: string[];
	custom?: boolean;
}

class Blocks {
	custom = $state<string[]>([]);
	/** domain → block. Expired blocks are dropped lazily. */
	list = $state<Record<string, Block>>({});
	attempts = $state<Attempt[]>([]);
	lastDuration = $state<DurationId>('today');
	/** What the last Block press changed, so a mis-click can be undone for 10 seconds. */
	recent = $state<{ at: number; before: Record<string, Block | null>; added: string[] } | null>(null);

	constructor() {
		if (typeof localStorage === 'undefined') return;
		try {
			const saved = JSON.parse(localStorage.getItem(KEY) ?? 'null');
			if (saved) Object.assign(this, saved);
		} catch {
			/* corrupted — start fresh */
		}
	}

	#save() {
		const { custom, list, attempts, lastDuration } = this;
		localStorage.setItem(KEY, JSON.stringify($state.snapshot({ custom, list, attempts, lastDuration })));
	}

	get categories(): Category[] {
		return [
			...PRESETS.map((g) => ({ id: g.group, name: g.group, sites: g.sites })),
			{ id: 'yours', name: 'Your sites', sites: this.custom, custom: true }
		];
	}

	get allSites() {
		return this.categories.flatMap((c) => c.sites);
	}

	/** The block in force on exactly this domain, if any. */
	of(domain: string): Block | undefined {
		const b = this.list[domain];
		if (!b) return undefined;
		return b.until === null || b.until > clock.now ? b : undefined;
	}

	/** The block in force for a URL: the longest one among matching domains. */
	forHref(href: string): (Block & { domain: string }) | undefined {
		if (isInternal(href)) return undefined;
		const host = hostOf(href);
		if (!host || isException(host)) return undefined;
		let best: (Block & { domain: string }) | undefined;
		for (const domain of Object.keys(this.list)) {
			const b = this.of(domain);
			if (b && matches(host, domain) && (!best || outlasts(b.until, best.until))) best = { ...b, domain };
		}
		return best;
	}

	blocks(href: string) {
		return !!this.forHref(href);
	}

	get active() {
		return Object.keys(this.list).filter((d) => this.of(d));
	}

	get any() {
		return this.active.length > 0;
	}

	/** The list entry covering a host (youtube.com for m.youtube.com), or the host itself. */
	entryFor(host: string) {
		return this.allSites.find((d) => matches(host, d)) ?? host;
	}

	/**
	 * Block these sites until `until`. Sites already blocked for longer keep their block.
	 * Sites not on the list yet join "Your sites".
	 */
	block(domains: string[], until: Until) {
		const now = clock.now;
		const added = domains.filter((d) => !this.allSites.includes(d));
		this.custom.push(...added);
		const before: Record<string, Block | null> = {};
		let changed = 0;
		for (const d of domains) {
			const cur = this.of(d);
			if (cur && outlasts(cur.until, until)) continue;
			before[d] = cur ? { ...cur } : null;
			this.list[d] = { since: cur?.since ?? now, until };
			changed++;
		}
		if (changed) this.recent = { at: now, before, added };
		this.#save();
		return changed;
	}

	get canUndo() {
		return !!this.recent && clock.now - this.recent.at < UNDO_MS;
	}

	undo() {
		if (!this.canUndo) return false;
		for (const [d, b] of Object.entries(this.recent!.before)) {
			if (b) this.list[d] = b;
			else delete this.list[d];
		}
		this.custom = this.custom.filter((d) => !this.recent!.added.includes(d));
		this.recent = null;
		this.#save();
		return true;
	}

	recordAttempt(href: string) {
		if (!this.blocks(href)) return;
		this.attempts.push({ at: clock.now, host: hostOf(href) });
		if (this.attempts.length > 500) this.attempts.splice(0, this.attempts.length - 500);
		this.#save();
	}

	stoppedFor(domain: string) {
		return this.attempts.filter((a) => matches(a.host, domain)).length;
	}

	stoppedBetween(from: number, to: number) {
		return this.attempts.filter((a) => a.at >= from && a.at < to).length;
	}

	get stoppedToday() {
		const day = startOfDay(clock.now);
		return this.stoppedBetween(day, day + DAY);
	}

	/** Adding is always allowed. */
	addSite(domain: string) {
		if (!domain || this.allSites.includes(domain)) return false;
		this.custom.push(domain);
		this.#save();
		return true;
	}

	/** Only sites that aren't blocked can be removed. */
	removeSite(domain: string) {
		if (this.of(domain)) return false;
		this.custom = this.custom.filter((d) => d !== domain);
		this.#save();
		return true;
	}

	setDuration(id: DurationId) {
		this.lastDuration = id;
		this.#save();
	}

	reset() {
		this.custom = [];
		this.list = {};
		this.attempts = [];
		this.recent = null;
		this.lastDuration = 'today';
		this.#save();
	}
}

export const blocks = new Blocks();
