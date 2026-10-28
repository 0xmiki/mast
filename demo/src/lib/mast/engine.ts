/** Pure helpers: what is blocked, and how to say times. */
import { hostOf, isInternal, matches } from './url';

export const SECOND = 1000;
export const MINUTE = 60 * SECOND;
export const HOUR = 60 * MINUTE;
export const DAY = 24 * HOUR;
/** Mis-click protection right after starting. Not a way out. */
export const UNDO_MS = 10 * SECOND;

export function isBlocked(href: string, blocklist: string[]) {
	if (isInternal(href)) return false;
	const host = hostOf(href);
	return !!host && blocklist.some((d) => matches(host, d));
}

export function dayKey(t: number) {
	const d = new Date(t);
	return `${d.getFullYear()}-${d.getMonth() + 1}-${d.getDate()}`;
}

export function startOfDay(t: number) {
	const d = new Date(t);
	d.setHours(0, 0, 0, 0);
	return d.getTime();
}

/** "1h 20m", "45m", "12s" */
export function formatDuration(ms: number): string {
	const s = Math.max(0, Math.ceil(ms / 1000));
	const h = Math.floor(s / 3600);
	const m = Math.floor((s % 3600) / 60);
	if (h) return m ? `${h}h ${m}m` : `${h}h`;
	if (m) return `${m}m`;
	return `${s}s`;
}

/** "01:24:09" or "24:09" */
export function countdown(ms: number) {
	const s = Math.max(0, Math.ceil(ms / 1000));
	const h = Math.floor(s / 3600);
	const mm = String(Math.floor((s % 3600) / 60)).padStart(2, '0');
	const ss = String(s % 60).padStart(2, '0');
	return h ? `${h}:${mm}:${ss}` : `${mm}:${ss}`;
}

export function clockTime(t: number) {
	return new Date(t).toLocaleTimeString([], { hour: '2-digit', minute: '2-digit' });
}

export function formatWhen(t: number, now: number): string {
	const time = clockTime(t);
	if (dayKey(t) === dayKey(now)) return `today ${time}`;
	if (dayKey(t) === dayKey(now + DAY)) return `tomorrow ${time}`;
	return new Date(t).toLocaleDateString([], { weekday: 'short', month: 'short', day: 'numeric' }) + ` ${time}`;
}
