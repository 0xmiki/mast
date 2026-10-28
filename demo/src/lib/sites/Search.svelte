<script lang="ts">
	import type { SiteProps } from './types';
	import { gradientFor, hash } from './types';

	let { url, navigate }: SiteProps = $props();

	type Hit = {
		domain: string;
		/** Breadcrumb shown under the domain, in green. */
		crumb: string;
		href: string;
		title: string;
		snippet: string;
	};

	const CATALOGUE: Hit[] = [
		{ domain: 'youtube.com', crumb: 'youtube.com', href: 'https://www.youtube.com/', title: 'YouTube', snippet: 'Enjoy the videos and music you love, upload original content, and share it with friends, family and the world.' },
		{ domain: 'youtube.com', crumb: 'youtube.com › watch?v=8fKq2LwZ', href: 'https://www.youtube.com/watch?v=8fKq2LwZ', title: 'Ulysses pact explained in 12 minutes - YouTube', snippet: 'A short video on precommitment, Odysseus and the Sirens, and why people buy gym memberships they never use.' },
		{ domain: 'x.com', crumb: 'x.com › mast_browser › status › 1849', href: 'https://x.com/mast_browser/status/1849', title: 'Mast on X: "Tab memory is live in 0.9. Your tabs now remember why you opened them."', snippet: 'Thread on tab memory, Clean pacts, and what we are cutting next. Replies welcome.' },
		{ domain: 'reddit.com', crumb: 'reddit.com › r › productivity › comments', href: 'https://www.reddit.com/r/productivity/comments/1ab2c3/', title: 'r/productivity - Does anyone else use precommitment? What worked for you?', snippet: 'I started putting my phone in another room before work and it changed my mornings. Curious what others have tried.' },
		{ domain: 'news.ycombinator.com', crumb: 'news.ycombinator.com › item?id=41033', href: 'https://news.ycombinator.com/item?id=41033', title: 'The Ulysses pact and precommitment devices | Hacker News', snippet: 'Discussion of commitment devices, Elster\'s book, and whether binding yourself is a rational thing to do.' },
		{ domain: 'github.com', crumb: 'github.com › mast-browser › mast', href: 'https://github.com/mast-browser/mast', title: 'mast-browser/mast: A productivity browser that quietly removes what you do not need', snippet: 'Chromium-based productivity browser with Clean pacts, tab memory and simulated sites. Build instructions are in the README.' },
		{ domain: 'en.wikipedia.org', crumb: 'en.wikipedia.org › wiki › Ulysses_pact', href: 'https://en.wikipedia.org/wiki/Ulysses_pact', title: 'Ulysses pact - Wikipedia', snippet: 'A Ulysses pact is a form of precommitment in which a person binds themselves to a course of action to stop a future self from choosing otherwise. The term comes from Homer\'s Odyssey.' },
		{ domain: 'en.wikipedia.org', crumb: 'en.wikipedia.org › wiki › Akrasia', href: 'https://en.wikipedia.org/wiki/Akrasia', title: 'Akrasia - Wikipedia', snippet: 'Akrasia, or weakness of will, is acting against one\'s own better judgement. It is a central topic in ancient ethics.' },
		{ domain: 'developer.mozilla.org', crumb: 'developer.mozilla.org › en-US › docs › Web', href: 'https://developer.mozilla.org/en-US/docs/Web', title: 'Web technology for developers | MDN', snippet: 'Learn about the open web: HTML, CSS, JavaScript, and the APIs that browsers implement for building sites and apps.' },
		{ domain: 'docs.python.org', crumb: 'docs.python.org › 3', href: 'https://docs.python.org/3/', title: 'Python 3 documentation', snippet: 'The official documentation for the Python language, including the tutorial, the standard library and language reference.' },
		{ domain: 'stackoverflow.com', crumb: 'stackoverflow.com › questions › 7731240', href: 'https://stackoverflow.com/questions/7731240/', title: 'How do I unregister a service worker for one origin only? - Stack Overflow', snippet: 'My service worker caches too aggressively. I want to unregister it for one origin without clearing storage for the rest.' },
		{ domain: 'chromium.googlesource.com', crumb: 'chromium.googlesource.com › chromium › src', href: 'https://chromium.googlesource.com/chromium/src', title: 'Chromium source code', snippet: 'Source code for the Chromium project, including the browser, the renderer and the build tools.' },
		{ domain: 'brave.com', crumb: 'brave.com › download', href: 'https://brave.com/download/', title: 'Brave Browser: Fast, private and free', snippet: 'Brave is a free and open source web browser that blocks ads and trackers by default and rewards you for browsing.' },
		{ domain: 'helium.computer', crumb: 'helium.computer', href: 'https://helium.computer/', title: 'Helium: a lightweight, privacy-focused browser', snippet: 'Helium is a Chromium-based browser focused on privacy, speed and a small footprint.' },
		{ domain: 'news.bbc.co.uk', crumb: 'news.bbc.co.uk', href: 'https://news.bbc.co.uk/', title: 'BBC News - Home', snippet: 'Breaking news, video and analysis from the BBC, covering the UK and the rest of the world.' },
		{ domain: 'kit.svelte.dev', crumb: 'kit.svelte.dev › docs', href: 'https://kit.svelte.dev/docs', title: 'Introduction · SvelteKit docs', snippet: 'SvelteKit is a framework for building web applications of all sizes, with a flexible filesystem-based router.' }
	];

	const q = $derived(url.searchParams.get('q')?.trim() ?? '');
	const words = $derived(q.toLowerCase().split(/\s+/).filter(Boolean));
	const total = $derived(q ? 12000 + (hash(q) % 4800000) : 0);

	/** Entries that mention any word of the query come first; the rest follow in catalogue order. */
	const results = $derived.by(() => {
		const matches = (h: Hit) => {
			const text = `${h.title} ${h.snippet} ${h.domain}`.toLowerCase();
			return words.some((w) => text.includes(w));
		};
		return [...CATALOGUE.filter(matches), ...CATALOGUE.filter((h) => !matches(h))].slice(0, 8);
	});

	function onSearch(e: Event) {
		e.preventDefault();
		const next = String(new FormData(e.currentTarget as HTMLFormElement).get('q') ?? '').trim();
		if (next) navigate('mast://search?q=' + encodeURIComponent(next));
	}
</script>

{#snippet searchBox(big: boolean)}
	<form class="box" class:big role="search" onsubmit={onSearch}>
		<input name="q" type="search" value={q} aria-label="Search" autocomplete="off" />
		<button type="submit" aria-label="Search">
			<svg viewBox="0 0 24 24" width="18" height="18" aria-hidden="true">
				<circle cx="11" cy="11" r="7" fill="none" stroke="currentColor" stroke-width="2" />
				<path d="M16.5 16.5 21 21" stroke="currentColor" stroke-width="2" stroke-linecap="round" />
			</svg>
		</button>
	</form>
{/snippet}

<div class="srch">
	{#if q}
		<header class="top">
			<span class="word">Mast Search</span>
			{@render searchBox(false)}
		</header>

		<div class="body">
			<p class="stat">About {total.toLocaleString('en-US')} results</p>
			<ol class="results">
				{#each results as r (r.href)}
					<li class="res">
						<div class="src">
							<span class="fav" style="background: {gradientFor(r.domain)}">{r.domain.charAt(0).toUpperCase()}</span>
							<div class="site">
								<div class="dom">{r.domain}</div>
								<div class="crumb">{r.crumb}</div>
							</div>
						</div>
						<a class="title" href={r.href}>{r.title}</a>
						<p class="snip">{r.snippet}</p>
					</li>
				{/each}
			</ol>
		</div>
	{:else}
		<div class="home">
			{@render searchBox(true)}
		</div>
	{/if}
</div>

<style>
	.srch {
		min-height: 100%;
		background: #fff;
		color: #1f1f1f;
		font-family: 'Inter Variable', ui-sans-serif, sans-serif;
		font-size: 14px;
		line-height: 1.5;
	}
	a {
		color: #1a0dab;
		text-decoration: none;
	}
	a:hover {
		text-decoration: underline;
	}

	.home {
		display: flex;
		align-items: center;
		justify-content: center;
		min-height: 100%;
		padding: 24px;
		box-sizing: border-box;
	}
	.top {
		display: flex;
		align-items: center;
		gap: 20px;
		padding: 14px 20px 12px clamp(16px, 8vw, 120px);
		border-bottom: 1px solid #ebebeb;
	}
	.word {
		flex: none;
		font-size: 16px;
		font-weight: 700;
		letter-spacing: -0.01em;
	}

	.box {
		display: flex;
		align-items: center;
		width: min(640px, 100%);
		height: 44px;
		padding: 0 6px 0 18px;
		box-sizing: border-box;
		border: 1px solid #dfe1e5;
		border-radius: 24px;
		box-shadow: 0 1px 6px rgb(32 33 36 / 0.12);
	}
	.box:focus-within {
		border-color: #cfd6e4;
		box-shadow: 0 1px 6px rgb(32 33 36 / 0.22);
	}
	.box.big {
		height: 52px;
		font-size: 16px;
	}
	.box input {
		flex: 1;
		min-width: 0;
		border: none;
		outline: none;
		background: transparent;
		color: #1f1f1f;
		font: inherit;
	}
	.box button {
		display: grid;
		place-items: center;
		width: 36px;
		height: 36px;
		border: none;
		border-radius: 50%;
		background: transparent;
		color: #4d5156;
		cursor: pointer;
	}
	.box button:hover {
		background: #f1f3f4;
	}

	.body {
		padding: 16px 20px 40px clamp(16px, 8vw, 120px);
		max-width: 760px;
	}
	.stat {
		margin: 0 0 16px;
		color: #70757a;
		font-size: 13px;
	}
	.results {
		margin: 0;
		padding: 0;
		list-style: none;
	}
	.res {
		max-width: 652px;
		margin-bottom: 26px;
	}
	.src {
		display: flex;
		align-items: center;
		gap: 10px;
	}
	.fav {
		display: grid;
		place-items: center;
		flex: none;
		width: 26px;
		height: 26px;
		border-radius: 50%;
		color: #fff;
		font-size: 12px;
		font-weight: 600;
	}
	.site {
		min-width: 0;
		line-height: 1.3;
	}
	.dom {
		color: #1f1f1f;
		font-size: 14px;
	}
	.crumb {
		overflow: hidden;
		color: #006621;
		font-size: 12px;
		text-overflow: ellipsis;
		white-space: nowrap;
	}
	.title {
		display: inline-block;
		margin-top: 4px;
		font-size: 20px;
		line-height: 1.3;
	}
	.snip {
		display: -webkit-box;
		margin: 4px 0 0;
		overflow: hidden;
		color: #4d4d4d;
		font-size: 14px;
		line-height: 1.58;
		line-clamp: 2;
		-webkit-line-clamp: 2;
		-webkit-box-orient: vertical;
	}
</style>
