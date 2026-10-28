/**
 * The whole product in one store: a blocklist, and sessions that block it.
 * A running session can be extended, never shortened or stopped
 * (apart from a 10-second undo for mis-clicks).
 */
import { clock } from './clock.svelte';
import { PRESET_SITES } from './blocklist';
import { UNDO_MS, MINUTE, isBlocked, startOfDay } from './engine';
import type { Attempt, Session } from './types';
import { hostOf } from './url';

const KEY = 'mast.v2';

class Focus {
	custom = $state<string[]>([]);
	sessions = $state<Session[]>([]);
	attempts = $state<Attempt[]>([]);
	/** Last chosen length, preselected next time. */
	lastMinutes = $state(50);

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
		const { custom, sessions, attempts, lastMinutes } = this;
		localStorage.setItem(KEY, JSON.stringify($state.snapshot({ custom, sessions, attempts, lastMinutes })));
	}

	get blocklist() {
		return [...PRESET_SITES, ...this.custom];
	}

	get active(): Session | undefined {
		const now = clock.now;
		return this.sessions.find((s) => s.startedAt <= now && now < s.endsAt);
	}

	get remaining() {
		const s = this.active;
		return s ? s.endsAt - clock.now : 0;
	}

	get canUndo() {
		const s = this.active;
		return !!s && clock.now - s.startedAt < UNDO_MS;
	}

	blocks(href: string) {
		return !!this.active && isBlocked(href, this.blocklist);
	}

	start(minutes: number) {
		if (this.active) return;
		const now = clock.now;
		this.sessions.push({
			id: Math.random().toString(36).slice(2, 10),
			startedAt: now,
			endsAt: now + minutes * MINUTE,
			extensions: []
		});
		this.lastMinutes = minutes;
		this.#save();
	}

	extend(minutes: number) {
		const s = this.active;
		if (!s) return;
		s.endsAt += minutes * MINUTE;
		s.extensions.push(clock.now);
		this.#save();
	}

	undo() {
		if (!this.canUndo) return false;
		const id = this.active!.id;
		this.sessions = this.sessions.filter((s) => s.id !== id);
		this.#save();
		return true;
	}

	recordAttempt(href: string) {
		if (!this.blocks(href)) return;
		this.attempts.push({ at: clock.now, host: hostOf(href) });
		if (this.attempts.length > 500) this.attempts.splice(0, this.attempts.length - 500);
		this.#save();
	}

	isCustom(domain: string) {
		return this.custom.includes(domain);
	}

	/** Adding is always allowed — even mid-session, it only makes things stricter. */
	addSite(domain: string) {
		if (!domain || this.blocklist.includes(domain)) return false;
		this.custom.push(domain);
		this.#save();
		return true;
	}

	/** Removing only between sessions. */
	removeSite(domain: string) {
		if (this.active) return false;
		this.custom = this.custom.filter((d) => d !== domain);
		this.#save();
		return true;
	}

	get focusedToday() {
		const now = clock.now;
		const day = startOfDay(now);
		let ms = 0;
		for (const s of this.sessions) ms += Math.max(0, Math.min(s.endsAt, now) - Math.max(s.startedAt, day));
		return ms;
	}

	get attemptsToday() {
		const day = startOfDay(clock.now);
		return this.attempts.filter((a) => a.at >= day).length;
	}

	attemptsThisSession(host?: string) {
		const s = this.active;
		if (!s) return 0;
		return this.attempts.filter((a) => a.at >= s.startedAt && (!host || a.host === host)).length;
	}

	reset() {
		this.custom = [];
		this.sessions = [];
		this.attempts = [];
		this.lastMinutes = 50;
		this.#save();
	}
}

export const focus = new Focus();
