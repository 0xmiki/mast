/** Parts of a site that a Clean pact can strip. */
export type CleanFeature = 'feed' | 'shorts' | 'recommendations' | 'comments';

export interface SiteProps {
	/** The URL the tab is showing. Read pathname / searchParams from it. */
	url: URL;
	/** Features the user's Clean pacts strip from this site. */
	clean: CleanFeature[];
	/** Navigate the tab. Use for forms; plain <a href> clicks are intercepted by the shell. */
	navigate: (href: string) => void;
}

/** Deterministic hash for picking colours / numbers from ids. */
export function hash(input: string): number {
	let h = 2166136261;
	for (let i = 0; i < input.length; i++) {
		h ^= input.charCodeAt(i);
		h = Math.imul(h, 16777619);
	}
	return h >>> 0;
}

/** A pleasant two-stop gradient derived from a seed, for fake thumbnails and avatars. */
export function gradientFor(seed: string): string {
	const h = hash(seed);
	const a = h % 360;
	const b = (a + 40 + ((h >> 9) % 80)) % 360;
	return `linear-gradient(135deg, hsl(${a} 70% 55%), hsl(${b} 65% 35%))`;
}
