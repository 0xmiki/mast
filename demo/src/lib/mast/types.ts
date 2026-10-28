import type { CleanFeature } from '../sites/types';

export type { CleanFeature };

export type PactKind = 'block' | 'budget' | 'focus' | 'friction' | 'clean';

/** Daily window. `from`/`to` are minutes since midnight; `to <= from` wraps past midnight. */
export interface Schedule {
	always: boolean;
	/** 0 = Sunday … 6 = Saturday */
	days: number[];
	from: number;
	to: number;
}

export type Rule =
	| { kind: 'block'; schedule: Schedule }
	| { kind: 'budget'; minutesPerDay: number }
	| { kind: 'focus' }
	| { kind: 'friction'; waitSeconds: number }
	| { kind: 'clean'; features: CleanFeature[] };

export type Escape = 'none' | 'cooldown';

export interface Amendment {
	at: number;
	text: string;
}

export interface Pact {
	id: string;
	/** The user's own words — shown back on the block screen. */
	reason: string;
	/** Domains. For focus pacts this is the allow-list. */
	sites: string[];
	rule: Rule;
	escape: Escape;
	createdAt: number;
	sealedAt: number;
	graceEndsAt: number;
	startsAt: number;
	endsAt: number;
	releaseRequestedAt?: number;
	amendments: Amendment[];
}

export type PactStatus = 'grace' | 'upcoming' | 'active' | 'releasing' | 'released' | 'kept';

export interface Attempt {
	at: number;
	host: string;
	pactId: string;
}

export type Verdict =
	| { kind: 'allow'; clean: CleanFeature[]; budget?: { pact: Pact; remainingMs: number } }
	| { kind: 'block'; pact: Pact; until: number }
	| { kind: 'focus-block'; pact: Pact; until: number }
	| { kind: 'budget-out'; pact: Pact; until: number }
	| { kind: 'friction'; pact: Pact; waitSeconds: number };

export interface Draft {
	kind: PactKind;
	reason: string;
	sites: string[];
	schedule: Schedule;
	minutesPerDay: number;
	waitSeconds: number;
	features: CleanFeature[];
	/** focus session length, minutes */
	sessionMinutes: number;
	/** duration in days for non-focus pacts */
	days: number;
	startsAt: 'now' | 'tomorrow';
	escape: Escape;
}
