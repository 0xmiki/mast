/** One deep-work session: everything on the blocklist is unreachable until it ends. */
export interface Session {
	id: string;
	startedAt: number;
	endsAt: number;
	/** Each "+30 min" while running. Sessions can only get longer. */
	extensions: number[];
}

export interface Attempt {
	at: number;
	host: string;
}
