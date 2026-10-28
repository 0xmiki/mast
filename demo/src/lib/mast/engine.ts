/**
 * Pure enforcement engine. Given a URL, the time and the sealed pacts, decide what the
 * browser does. The Chromium fork ports this to a NavigationThrottle + C++ policy.
 */
import type { CleanFeature, Pact, PactStatus, Schedule, Verdict } from './types';
import { hostOf, isInternal, matches } from './url';

export const MINUTE = 60_000;
export const HOUR = 60 * MINUTE;
export const DAY = 24 * HOUR;
export const GRACE_MS = 2 * MINUTE;
export const RELEASE_MS = DAY;

export const DAY_NAMES = ['Sun', 'Mon', 'Tue', 'Wed', 'Thu', 'Fri', 'Sat'];
export const FEATURE_LABELS: Record<CleanFeature, string> = {
	feed: 'home feeds',
	shorts: 'Shorts',
	recommendations: 'recommendations',
	comments: 'comments'
};

export function status(p: Pact, now: number): PactStatus {
	if (p.releaseRequestedAt && now >= p.releaseRequestedAt + RELEASE_MS) return 'released';
	if (now >= p.endsAt) return 'kept';
	if (now < p.graceEndsAt) return 'grace';
	if (now < p.startsAt) return 'upcoming';
	if (p.releaseRequestedAt) return 'releasing';
	return 'active';
}

/** The pact is in force (sealed, started, not ended or released). */
export function isLive(p: Pact, now: number) {
	const s = status(p, now);
	return (s === 'grace' || s === 'active' || s === 'releasing') && now >= p.startsAt;
}

export function isOpen(p: Pact, now: number) {
	const s = status(p, now);
	return s !== 'kept' && s !== 'released';
}

function minutesOfDay(d: Date) {
	return d.getHours() * 60 + d.getMinutes();
}

function at(base: number, minutes: number, dayOffset = 0) {
	const d = new Date(base);
	d.setHours(0, 0, 0, 0);
	d.setDate(d.getDate() + dayOffset);
	return d.getTime() + minutes * MINUTE;
}

export function inWindow(s: Schedule, now: number) {
	if (s.always) return true;
	const d = new Date(now);
	const m = minutesOfDay(d);
	if (s.from < s.to) return s.days.includes(d.getDay()) && m >= s.from && m < s.to;
	// overnight window: started today, or started yesterday and still running
	const yesterday = (d.getDay() + 6) % 7;
	return (s.days.includes(d.getDay()) && m >= s.from) || (s.days.includes(yesterday) && m < s.to);
}

/** When the current window closes (assumes inWindow). */
export function windowEnd(s: Schedule, now: number) {
	if (s.always) return Infinity;
	const m = minutesOfDay(new Date(now));
	if (s.from < s.to || m >= s.from) return at(now, s.to, s.from < s.to ? 0 : 1);
	return at(now, s.to);
}

/** Next time a window opens, within the coming week. */
export function nextWindowStart(s: Schedule, now: number): number | null {
	if (s.always) return now;
	for (let i = 0; i < 8; i++) {
		const t = at(now, s.from, i);
		if (t > now && s.days.includes(new Date(t).getDay())) return t;
	}
	return null;
}

/** Is this pact restricting anything right now? */
export function enforcing(p: Pact, now: number) {
	if (!isLive(p, now)) return false;
	return p.rule.kind !== 'block' || inWindow(p.rule.schedule, now);
}

export function dayKey(now: number) {
	const d = new Date(now);
	return `${d.getFullYear()}-${d.getMonth() + 1}-${d.getDate()}`;
}

export function nextMidnight(now: number) {
	return at(now, 0, 1);
}

export function usageKey(p: Pact, now: number) {
	return `${p.id}|${dayKey(now)}`;
}

export function covers(p: Pact, host: string) {
	return p.sites.some((d) => matches(host, d));
}

export interface EngineContext {
	usage: Record<string, number>;
	frictionPasses: Record<string, number>;
}

export function frictionKey(p: Pact, host: string) {
	return `${p.id}|${host}`;
}

const RANK: Record<Verdict['kind'], number> = {
	block: 4,
	'focus-block': 4,
	'budget-out': 3,
	friction: 2,
	allow: 0
};

export function evaluate(href: string, now: number, pacts: Pact[], ctx: EngineContext): Verdict {
	const allow: Verdict & { kind: 'allow' } = { kind: 'allow', clean: [] };
	if (isInternal(href)) return allow;
	const host = hostOf(href);
	let worst = allow as Verdict;
	const consider = (v: Verdict) => {
		if (RANK[v.kind] > RANK[worst.kind]) worst = v;
	};

	for (const p of pacts) {
		if (!enforcing(p, now)) continue;
		const r = p.rule;
		if (r.kind === 'focus') {
			if (!covers(p, host)) consider({ kind: 'focus-block', pact: p, until: p.endsAt });
			continue;
		}
		if (!covers(p, host)) continue;
		switch (r.kind) {
			case 'block':
				consider({ kind: 'block', pact: p, until: Math.min(windowEnd(r.schedule, now), p.endsAt) });
				break;
			case 'budget': {
				const used = ctx.usage[usageKey(p, now)] ?? 0;
				const remainingMs = r.minutesPerDay * MINUTE - used;
				if (remainingMs <= 0) {
					consider({ kind: 'budget-out', pact: p, until: Math.min(nextMidnight(now), p.endsAt) });
				} else if (!allow.budget || remainingMs < allow.budget.remainingMs) {
					allow.budget = { pact: p, remainingMs };
				}
				break;
			}
			case 'friction': {
				if ((ctx.frictionPasses[frictionKey(p, host)] ?? 0) > now) break;
				const stronger =
					worst.kind === 'friction' ? r.waitSeconds > worst.waitSeconds : RANK.friction > RANK[worst.kind];
				if (stronger) worst = { kind: 'friction', pact: p, waitSeconds: r.waitSeconds };
				break;
			}
			case 'clean':
				for (const f of r.features) if (!allow.clean.includes(f)) allow.clean.push(f);
				break;
		}
	}
	return worst;
}

// ---------------------------------------------------------------- language

export function formatDuration(ms: number): string {
	if (!Number.isFinite(ms)) return 'forever';
	const s = Math.max(0, Math.ceil(ms / 1000));
	const d = Math.floor(s / 86400);
	const h = Math.floor((s % 86400) / 3600);
	const m = Math.floor((s % 3600) / 60);
	const sec = s % 60;
	if (d) return h ? `${d}d ${h}h` : `${d}d`;
	if (h) return m ? `${h}h ${m}m` : `${h}h`;
	if (m) return m < 10 && sec ? `${m}m ${sec}s` : `${m}m`;
	return `${sec}s`;
}

export function clockTime(minutes: number) {
	const h = Math.floor(minutes / 60) % 24;
	const m = minutes % 60;
	return `${String(h).padStart(2, '0')}:${String(m).padStart(2, '0')}`;
}

export function formatWhen(t: number, now: number): string {
	if (!Number.isFinite(t)) return 'never';
	const d = new Date(t);
	const time = d.toLocaleTimeString([], { hour: '2-digit', minute: '2-digit' });
	if (dayKey(t) === dayKey(now)) return `today ${time}`;
	if (dayKey(t) === dayKey(now + DAY)) return `tomorrow ${time}`;
	if (t - now < 6 * DAY) return `${d.toLocaleDateString([], { weekday: 'long' })} ${time}`;
	return d.toLocaleDateString([], { month: 'short', day: 'numeric' }) + ` ${time}`;
}

export function formatDate(t: number) {
	return new Date(t).toLocaleDateString([], { month: 'short', day: 'numeric' });
}

export function daysPhrase(days: number[]) {
	const set = [...days].sort();
	if (set.length === 7) return 'every day';
	if (set.join() === '1,2,3,4,5') return 'every weekday';
	if (set.join() === '0,6') return 'on weekends';
	return 'on ' + set.map((d) => DAY_NAMES[d]).join(', ');
}

export function schedulePhrase(s: Schedule) {
	if (s.always) return 'at all times';
	return `${daysPhrase(s.days)}, ${clockTime(s.from)}–${clockTime(s.to)}`;
}

export function listPhrase(items: string[]) {
	if (items.length <= 1) return items[0] ?? 'nothing';
	return items.slice(0, -1).join(', ') + ' and ' + items[items.length - 1];
}

export const KIND_LABELS = {
	block: 'Block',
	budget: 'Budget',
	focus: 'Focus',
	friction: 'Speed bump',
	clean: 'Clean'
} as const;

/** One plain-language sentence for the whole pact. */
export function describe(p: Pick<Pact, 'rule' | 'sites'>): string {
	const sites = listPhrase(p.sites);
	const r = p.rule;
	switch (r.kind) {
		case 'block':
			return `${sites} will not open ${schedulePhrase(r.schedule)}.`;
		case 'budget':
			return `${r.minutesPerDay} minutes a day, shared across ${sites}. Then blocked until midnight.`;
		case 'focus':
			return `Only ${sites} will open. Everything else is sealed.`;
		case 'friction':
			return `A ${r.waitSeconds}-second wait before ${sites} opens.`;
		case 'clean':
			return `${listPhrase(r.features.map((f) => FEATURE_LABELS[f]))} hidden on ${sites}.`;
	}
}

/** What the pact is doing right now, in one short line. */
export function ruleLine(p: Pact, now: number, usedMs = 0): string {
	const s = status(p, now);
	if (s === 'kept') return `Kept · ended ${formatDate(p.endsAt)}`;
	if (s === 'released') return 'Released early · recorded as broken';
	if (now < p.startsAt) return `Starts ${formatWhen(p.startsAt, now)}`;
	const r = p.rule;
	switch (r.kind) {
		case 'block':
			if (inWindow(r.schedule, now)) {
				const until = Math.min(windowEnd(r.schedule, now), p.endsAt);
				return r.schedule.always ? `Sealed until ${formatWhen(until, now)}` : `Sealed now · opens ${formatWhen(until, now)}`;
			} else {
				const next = nextWindowStart(r.schedule, now);
				return next && next < p.endsAt ? `Open now · seals ${formatWhen(next, now)}` : 'Open now';
			}
		case 'budget':
			return `${Math.floor(usedMs / MINUTE)} of ${r.minutesPerDay} min used today`;
		case 'focus':
			return `${formatDuration(p.endsAt - now)} left in session`;
		case 'friction':
			return `${r.waitSeconds}s wait before every visit`;
		case 'clean':
			return `Hiding ${listPhrase(r.features.map((f) => FEATURE_LABELS[f]))}`;
	}
}

/** 0..1 progress through the pact's whole term. */
export function progress(p: Pact, now: number) {
	return Math.min(1, Math.max(0, (now - p.startsAt) / (p.endsAt - p.startsAt)));
}
