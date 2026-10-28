/**
 * The pact store. Every mutation here either creates a pact or makes one stricter —
 * there is deliberately no update/delete path for sealed pacts (except the 2-minute
 * regret window).
 */
import { clock } from './clock.svelte';
import {
	DAY,
	GRACE_MS,
	HOUR,
	MINUTE,
	clockTime,
	covers,
	evaluate,
	frictionKey,
	isLive,
	isOpen,
	nextMidnight,
	status,
	usageKey,
	DAY_NAMES,
	FEATURE_LABELS
} from './engine';
import type { Attempt, CleanFeature, Draft, Pact } from './types';
import { hostOf } from './url';

const KEY = 'mast.v1';

interface Saved {
	pacts: Pact[];
	attempts: Attempt[];
	usage: Record<string, number>;
	frictionPasses: Record<string, number>;
}

function uid() {
	return Math.random().toString(36).slice(2, 10);
}

export function emptyDraft(): Draft {
	return {
		kind: 'block',
		reason: '',
		sites: [],
		schedule: { always: false, days: [1, 2, 3, 4, 5], from: 9 * 60, to: 17 * 60 },
		minutesPerDay: 20,
		waitSeconds: 30,
		features: ['shorts', 'recommendations'],
		sessionMinutes: 50,
		days: 30,
		startsAt: 'now',
		escape: 'none'
	};
}

export function draftToPact(d: Draft, now: number): Pact {
	const startsAt = d.kind !== 'focus' && d.startsAt === 'tomorrow' ? nextMidnight(now) : now;
	const endsAt = d.kind === 'focus' ? now + d.sessionMinutes * MINUTE : startsAt + d.days * DAY;
	const rule: Pact['rule'] =
		d.kind === 'block'
			? { kind: 'block', schedule: structuredClone($state.snapshot(d.schedule)) }
			: d.kind === 'budget'
				? { kind: 'budget', minutesPerDay: d.minutesPerDay }
				: d.kind === 'friction'
					? { kind: 'friction', waitSeconds: d.waitSeconds }
					: d.kind === 'clean'
						? { kind: 'clean', features: [...d.features] }
						: { kind: 'focus' };
	return {
		id: uid(),
		reason: d.reason.trim(),
		sites: [...d.sites],
		rule,
		escape: d.escape,
		createdAt: now,
		sealedAt: now,
		graceEndsAt: now + GRACE_MS,
		startsAt,
		endsAt,
		amendments: []
	};
}

class PactStore {
	pacts = $state<Pact[]>([]);
	attempts = $state<Attempt[]>([]);
	usage = $state<Record<string, number>>({});
	frictionPasses = $state<Record<string, number>>({});

	constructor() {
		if (typeof localStorage === 'undefined') return;
		try {
			const saved: Saved | null = JSON.parse(localStorage.getItem(KEY) ?? 'null');
			if (saved) Object.assign(this, saved);
		} catch {
			/* corrupted store — start fresh */
		}
	}

	#save() {
		const data: Saved = $state.snapshot({
			pacts: this.pacts,
			attempts: this.attempts,
			usage: this.usage,
			frictionPasses: this.frictionPasses
		});
		localStorage.setItem(KEY, JSON.stringify(data));
	}

	get(id: string) {
		return this.pacts.find((p) => p.id === id);
	}

	/** Pacts not yet kept/released, newest first. */
	get open() {
		const now = clock.now;
		return this.pacts.filter((p) => isOpen(p, now)).sort((a, b) => b.sealedAt - a.sealedAt);
	}

	get live() {
		const now = clock.now;
		return this.pacts.filter((p) => isLive(p, now));
	}

	get closed() {
		const now = clock.now;
		return this.pacts.filter((p) => !isOpen(p, now)).sort((a, b) => b.endsAt - a.endsAt);
	}

	/** Browser-wide lockdown: anything sealed and running locks the escape hatches. */
	get locked() {
		return this.live.length > 0;
	}

	get focus() {
		const now = clock.now;
		return this.live.find((p) => p.rule.kind === 'focus' && now < p.endsAt);
	}

	verdict(href: string) {
		return evaluate(href, clock.now, this.pacts, this);
	}

	attemptsToday(host?: string) {
		const start = nextMidnight(clock.now) - DAY;
		return this.attempts.filter((a) => a.at >= start && (!host || a.host === host)).length;
	}

	usedToday(p: Pact) {
		return this.usage[usageKey(p, clock.now)] ?? 0;
	}

	// ------------------------------------------------------------ lifecycle

	seal(d: Draft) {
		const pact = draftToPact(d, clock.now);
		this.pacts.push(pact);
		this.#save();
		return pact;
	}

	/** Only possible inside the regret window. */
	cancel(id: string) {
		const p = this.get(id);
		if (!p || status(p, clock.now) !== 'grace') return false;
		this.pacts = this.pacts.filter((x) => x.id !== id);
		this.#save();
		return true;
	}

	requestRelease(id: string) {
		const p = this.get(id);
		if (!p || p.escape !== 'cooldown' || p.releaseRequestedAt) return;
		p.releaseRequestedAt = clock.now;
		this.#amend(p, 'Release requested. Takes effect in 24 hours; recorded as broken.');
	}

	// ------------------------------------------------------------ enforcement events

	recordAttempt(href: string) {
		const v = this.verdict(href);
		if (v.kind === 'allow' || v.kind === 'friction') return;
		this.attempts.push({ at: clock.now, host: hostOf(href), pactId: v.pact.id });
		if (this.attempts.length > 500) this.attempts.splice(0, this.attempts.length - 500);
		this.#save();
	}

	passFriction(p: Pact, href: string) {
		this.frictionPasses[frictionKey(p, hostOf(href))] = clock.now + 10 * MINUTE;
		this.#save();
	}

	/** Called every second for the visible tab. */
	tickUsage(href: string, ms: number) {
		const host = hostOf(href);
		if (!host || this.verdict(href).kind !== 'allow') return;
		let touched = false;
		for (const p of this.live) {
			if (p.rule.kind !== 'budget' || !covers(p, host)) continue;
			const k = usageKey(p, clock.now);
			this.usage[k] = (this.usage[k] ?? 0) + ms;
			touched = true;
		}
		if (touched) this.#save();
	}

	// ------------------------------------------------------------ the ratchet (stricter only)

	#amend(p: Pact, text: string) {
		p.amendments.push({ at: clock.now, text });
		this.#save();
	}

	addSite(p: Pact, domain: string) {
		if (p.rule.kind === 'focus' || !domain || p.sites.includes(domain)) return;
		p.sites.push(domain);
		this.#amend(p, `Added ${domain}.`);
	}

	/** Focus pacts only: shrinking the allow-list is stricter. */
	removeAllowed(p: Pact, domain: string) {
		if (p.rule.kind !== 'focus' || p.sites.length <= 1) return;
		p.sites = p.sites.filter((s) => s !== domain);
		this.#amend(p, `Removed ${domain} from the allow-list.`);
	}

	extend(p: Pact, ms: number) {
		p.endsAt += ms;
		const label = ms >= DAY ? `${Math.round(ms / DAY)} day(s)` : `${Math.round(ms / MINUTE)} minutes`;
		this.#amend(p, `Extended by ${label}.`);
	}

	addDay(p: Pact, day: number) {
		if (p.rule.kind !== 'block' || p.rule.schedule.days.includes(day)) return;
		p.rule.schedule.days.push(day);
		this.#amend(p, `Now also blocks on ${DAY_NAMES[day]}.`);
	}

	widen(p: Pact, edge: 'earlier' | 'later') {
		if (p.rule.kind !== 'block') return;
		const s = p.rule.schedule;
		if (s.always || s.to <= s.from) return;
		if (edge === 'earlier' && s.from >= 60) s.from -= 60;
		else if (edge === 'later' && s.to <= 23 * 60) s.to += 60;
		else return;
		this.#amend(p, `Window widened to ${clockTime(s.from)}–${clockTime(s.to)}.`);
	}

	alwaysOn(p: Pact) {
		if (p.rule.kind !== 'block' || p.rule.schedule.always) return;
		p.rule.schedule.always = true;
		this.#amend(p, 'Now blocks at all times.');
	}

	lowerBudget(p: Pact, minutes: number) {
		if (p.rule.kind !== 'budget' || p.rule.minutesPerDay <= 0) return;
		p.rule.minutesPerDay = Math.max(0, p.rule.minutesPerDay - minutes);
		this.#amend(p, `Daily budget lowered to ${p.rule.minutesPerDay} minutes.`);
	}

	lengthenWait(p: Pact, seconds: number) {
		if (p.rule.kind !== 'friction') return;
		p.rule.waitSeconds += seconds;
		this.#amend(p, `Wait lengthened to ${p.rule.waitSeconds} seconds.`);
	}

	addFeature(p: Pact, f: CleanFeature) {
		if (p.rule.kind !== 'clean' || p.rule.features.includes(f)) return;
		p.rule.features.push(f);
		this.#amend(p, `Now also hides ${FEATURE_LABELS[f]}.`);
	}

	dropEscape(p: Pact) {
		if (p.escape !== 'cooldown' || p.releaseRequestedAt) return;
		p.escape = 'none';
		this.#amend(p, 'Escape clause removed. No way out.');
	}

	// ------------------------------------------------------------ demo only

	loadScenario() {
		const now = clock.now;
		const past = now - 3 * DAY;
		const mk = (d: Partial<Draft>, sealedAt = past): Pact => {
			const p = draftToPact({ ...emptyDraft(), ...d }, sealedAt);
			return p;
		};
		const work = mk({
			kind: 'block',
			reason: 'My best hours go to the work I will be proud of, not the feed.',
			sites: ['youtube.com', 'x.com', 'reddit.com'],
			days: 30
		});
		const budget = mk({
			kind: 'budget',
			reason: 'Twenty minutes is plenty to catch up.',
			sites: ['news.ycombinator.com', 'reddit.com'],
			minutesPerDay: 20,
			days: 14
		});
		const clean = mk({
			kind: 'clean',
			reason: 'YouTube for learning, not for Shorts.',
			sites: ['youtube.com'],
			features: ['shorts', 'recommendations', 'comments'],
			days: 90
		});
		const bump = mk({
			kind: 'friction',
			reason: 'Pause before the scroll.',
			sites: ['x.com', 'reddit.com'],
			waitSeconds: 20,
			days: 21,
			escape: 'cooldown'
		});
		const old = mk(
			{ kind: 'block', reason: 'Exam week.', sites: ['youtube.com'], days: 7, schedule: { ...emptyDraft().schedule, always: true } },
			now - 20 * DAY
		);
		work.amendments.push({ at: past + 4 * HOUR, text: 'Added reddit.com.' });
		this.pacts = [old, clean, budget, bump, work];
		this.usage = { [usageKey(budget, now)]: 14 * MINUTE };
		this.attempts = [
			{ at: now - 3 * HOUR, host: 'x.com', pactId: work.id },
			{ at: now - 2 * HOUR, host: 'youtube.com', pactId: work.id },
			{ at: now - 90 * MINUTE, host: 'x.com', pactId: work.id },
			{ at: now - 40 * MINUTE, host: 'reddit.com', pactId: work.id }
		];
		this.frictionPasses = {};
		this.#save();
	}

	reset() {
		this.pacts = [];
		this.attempts = [];
		this.usage = {};
		this.frictionPasses = {};
		this.#save();
	}
}

export const pacts = new PactStore();
