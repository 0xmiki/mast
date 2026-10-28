/** URL helpers shared by the omnibox, tabs and the engine. */

export const INTERNAL = 'mast:';

export function isInternal(href: string) {
	return href.startsWith('mast://');
}

/** Turn omnibox input into a URL: internal page, website, or search. */
export function normalizeInput(input: string): string {
	const text = input.trim();
	if (!text) return 'mast://newtab';
	if (isInternal(text)) return text;
	if (/^https?:\/\//i.test(text)) return text;
	if (/^[\w-]+(\.[\w-]+)+(:\d+)?(\/\S*)?$/.test(text) && !text.includes(' ')) {
		return 'https://' + text;
	}
	return 'mast://search?q=' + encodeURIComponent(text);
}

/** Parse any tab URL (including mast://) into a URL object. */
export function parse(href: string): URL {
	try {
		return new URL(href);
	} catch {
		return new URL('mast://newtab');
	}
}

/** Hostname without `www.` / `m.` — the key the blocklist matches against. */
export function hostOf(href: string): string {
	if (isInternal(href)) return '';
	const host = parse(href).hostname.toLowerCase();
	return host.replace(/^(www|m|mobile)\./, '');
}

/** Clean user input like "https://www.YouTube.com/feed" into "youtube.com". */
export function cleanDomain(input: string): string {
	let text = input.trim().toLowerCase();
	if (!text) return '';
	if (!/^[a-z]+:\/\//.test(text)) text = 'https://' + text;
	try {
		return new URL(text).hostname.replace(/^(www|m|mobile)\./, '');
	} catch {
		return '';
	}
}

/** Does `host` fall under `domain` (exact or subdomain)? */
export function matches(host: string, domain: string) {
	return host === domain || host.endsWith('.' + domain);
}

/** Human title for a tab. */
export function titleFor(href: string): string {
	if (isInternal(href)) {
		const url = parse(href);
		const page = url.host || url.pathname.replace(/^\/+/, '');
		if (page === 'search') return url.searchParams.get('q') || 'Search';
		return (
			{
				newtab: 'New Tab',
				blocklist: 'Blocklist',
				settings: 'Settings',
				history: 'History'
			}[page] ?? 'Mast'
		);
	}
	const host = hostOf(href);
	return (
		{
			'youtube.com': 'YouTube',
			'x.com': 'X',
			'twitter.com': 'X',
			'reddit.com': 'Reddit',
			'news.ycombinator.com': 'Hacker News',
			'github.com': 'GitHub',
			'en.wikipedia.org': 'Wikipedia'
		}[host] ?? host
	);
}

/** Display form for the omnibox when not focused. */
export function displayUrl(href: string) {
	if (href === 'mast://newtab') return '';
	return href.replace(/^https:\/\//, '').replace(/\/$/, '');
}
