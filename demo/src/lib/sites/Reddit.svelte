<script lang="ts">
	import type { SiteProps } from './types';
	import { gradientFor, hash } from './types';
	import CleanedNotice from './CleanedNotice.svelte';
	import { subs, posts, comments } from './reddit-data';
	import type { RPost, Sub } from './reddit-data';

	let { url, clean, navigate }: SiteProps = $props();

	/* Icon paths (24px grid). Votes are filled, the rest stroked. */
	const P = {
		home: 'M3 10.5 12 3l9 7.5V21h-6v-6H9v6H3z',
		popular: 'M4 17l5-5 4 4 7-8M15 8h5v5',
		all: 'M4 4h6v6H4zM14 4h6v6h-6zM4 14h6v6H4zM14 14h6v6h-6z',
		search: 'M10.5 3.5a7 7 0 1 0 0 14 7 7 0 0 0 0-14zM20.5 20.5l-4.6-4.6',
		up: 'M12 4 20 13h-5v7H9v-7H4z',
		down: 'M12 20 4 11h5V4h6v7h5z',
		comment: 'M4 5h16v11H9l-5 4z',
		share: 'M12 3v12M7 8l5-5 5 5M5 14v5h14v-5',
		save: 'M6 3h12v18l-6-4.5L6 21z',
		back: 'M15 5l-7 7 7 7'
	};

	const SORTS = ['Best', 'Hot', 'New', 'Top'];

	type Route =
		| { kind: 'home' }
		| { kind: 'search' }
		| { kind: 'other'; name: string }
		| { kind: 'sub'; sub: string }
		| { kind: 'post'; sub: string; id: string };

	function safeDecode(s: string): string {
		try {
			return decodeURIComponent(s);
		} catch {
			return s;
		}
	}

	const parts = $derived(url.pathname.split('/').filter(Boolean).map(safeDecode));

	const route = $derived.by((): Route => {
		const [first, second, third, fourth] = parts;
		if (!first) return { kind: 'home' };
		if (first === 'search') return { kind: 'search' };
		if (first === 'r' && second) {
			if (third === 'comments' && fourth) return { kind: 'post', sub: second, id: fourth };
			return { kind: 'sub', sub: second };
		}
		return { kind: 'other', name: first };
	});

	const term = $derived((url.searchParams.get('q') ?? '').trim());

	const hideFeed = $derived(clean.includes('feed'));
	const hideRecs = $derived(clean.includes('recommendations'));
	const hideComments = $derived(clean.includes('comments'));

	const sameName = (a: string, b: string) => a.toLowerCase() === b.toLowerCase();

	const searchResults = $derived(
		term
			? posts.filter((p) => {
					const needle = term.toLowerCase();
					return (
						p.title.toLowerCase().includes(needle) ||
						(p.body ?? '').toLowerCase().includes(needle)
					);
				})
			: []
	);

	const subInfo = $derived.by((): Sub | null => {
		if (route.kind !== 'sub') return null;
		const found = subs.find((s) => sameName(s.name, route.sub));
		if (found) return found;
		return {
			name: route.sub,
			members: `${10 + (hash(route.sub) % 900)}K`,
			about: `A community for everything about ${route.sub}.`
		};
	});

	const subPosts = $derived.by((): RPost[] => {
		if (route.kind !== 'sub') return [];
		const sub = route.sub;
		const matches = posts.filter((p) => sameName(p.sub, sub));
		return matches.length ? matches : posts;
	});

	/** The list rendered under the sort row on home, sub and search pages. */
	const listing = $derived.by((): RPost[] => {
		if (route.kind === 'search') return searchResults;
		if (route.kind === 'sub') return subPosts;
		return posts;
	});

	const postItem = $derived.by((): RPost | null => {
		if (route.kind !== 'post') return null;
		const id = route.id;
		return posts.find((p) => p.id === id) ?? null;
	});

	function fmt(n: number): string {
		const trim = (x: number) => (x >= 100 ? x.toFixed(0) : x.toFixed(1)).replace(/\.0$/, '');
		if (n >= 1_000_000) return trim(n / 1_000_000) + 'M';
		if (n >= 1_000) return trim(n / 1_000) + 'K';
		return String(n);
	}

	function submitSearch(e: SubmitEvent & { currentTarget: HTMLFormElement }) {
		e.preventDefault();
		const q = String(new FormData(e.currentTarget).get('q') ?? '').trim();
		if (q) navigate('/search?q=' + encodeURIComponent(q));
	}
</script>

{#snippet icon(d: string)}
	<svg class="ic" viewBox="0 0 24 24" aria-hidden="true"><path d={d} /></svg>
{/snippet}

{#snippet dot(name: string, cls: string = '')}
	<span class="dot {cls}" style="background: {gradientFor(name)}" aria-hidden="true">
		{name.charAt(0).toUpperCase()}
	</span>
{/snippet}

{#snippet sortRow()}
	<div class="sort" role="toolbar" aria-label="Sort posts">
		{#each SORTS as s, i (s)}
			<button type="button" class="sort-btn" class:on={i === 0}>{s}</button>
		{/each}
	</div>
{/snippet}

{#snippet card(p: RPost, full: boolean = false)}
	<article class="post" class:full>
		{#if !full}
			<a class="stretch" href="/r/{p.sub}/comments/{p.id}" aria-label={p.title}></a>
		{/if}
		<div class="votes">
			<button type="button" class="vote up" aria-label="Upvote">{@render icon(P.up)}</button>
			<span class="score">{p.votes}</span>
			<button type="button" class="vote down" aria-label="Downvote">{@render icon(P.down)}</button>
		</div>
		<div class="content">
			<div class="line">
				{@render dot(p.sub)}
				<a class="subname" href="/r/{p.sub}">r/{p.sub}</a>
				<span class="sep">•</span>
				<span>Posted by u/{p.author}</span>
				<span class="sep">•</span>
				<span>{p.age}</span>
			</div>
			<h3 class="title">{p.title}</h3>
			{#if p.flair}
				<span class="flair">{p.flair}</span>
			{/if}
			{#if p.media}
				<div class="media" style="background: {gradientFor(p.id)}"></div>
			{:else if p.body}
				<p class="preview">{p.body}</p>
			{/if}
			<footer class="foot">
				<button type="button" class="pillbtn">
					{@render icon(P.comment)}<span>{fmt(p.comments)} Comments</span>
				</button>
				<button type="button" class="pillbtn">
					{@render icon(P.share)}<span>Share</span>
				</button>
				<button type="button" class="pillbtn">
					{@render icon(P.save)}<span>Save</span>
				</button>
			</footer>
		</div>
	</article>
{/snippet}

<div class="rd">
	<header class="bar">
		<a class="brand" href="/" aria-label="reddit home">
			<span class="mark" aria-hidden="true"></span>
			<span class="word">reddit</span>
		</a>
		<form class="search" role="search" onsubmit={submitSearch}>
			{@render icon(P.search)}
			<input
				name="q"
				type="search"
				placeholder="Search Reddit"
				aria-label="Search Reddit"
				autocomplete="off"
				value={term}
			/>
		</form>
		<button type="button" class="login">Log In</button>
	</header>

	<div class="body">
		<aside class="side">
			<nav class="menu" aria-label="Feeds">
				<a class:on={route.kind === 'home'} href="/">{@render icon(P.home)}<span>Home</span></a>
				<a href="/popular">{@render icon(P.popular)}<span>Popular</span></a>
				<a href="/all">{@render icon(P.all)}<span>All</span></a>
			</nav>
			<div class="label">Communities</div>
			<nav class="menu" aria-label="Communities">
				{#each subs as s (s.name)}
					<a
						class:on={route.kind === 'sub' && sameName(route.sub, s.name)}
						href="/r/{s.name}"
					>
						{@render dot(s.name, 'sm')}<span>r/{s.name}</span>
					</a>
				{/each}
			</nav>
		</aside>

		<main class="center">
			{#if route.kind === 'home'}
				{@render sortRow()}
				{#if hideFeed}
					<div class="calm">
						<p>Feed hidden. Search for a specific question instead.</p>
						<div class="slot">
							<CleanedNotice label="Feed" dark />
						</div>
					</div>
				{:else}
					{#each listing as p (p.id)}
						{@render card(p)}
					{/each}
				{/if}
			{:else if route.kind === 'sub' && subInfo}
				<header class="sub-card">
					<div class="sub-banner" style="background: {gradientFor(subInfo.name)}"></div>
					<div class="sub-body">
						<div class="sub-top">
							<div class="sub-icon" style="background: {gradientFor(subInfo.name)}">
								{subInfo.name.charAt(0).toUpperCase()}
							</div>
							<button type="button" class="join">Join</button>
						</div>
						<h1>r/{subInfo.name}</h1>
						<div class="members">{subInfo.members} members</div>
						<p class="about">{subInfo.about}</p>
					</div>
				</header>
				{@render sortRow()}
				{#if hideFeed}
					<div class="calm">
						<p>Feed hidden. Search for a specific question instead.</p>
						<div class="slot">
							<CleanedNotice label="Feed" dark />
						</div>
					</div>
				{:else}
					{#each listing as p (p.id)}
						{@render card(p)}
					{/each}
				{/if}
			{:else if route.kind === 'search'}
				<div class="top-title">
					<strong>{term ? `Results for “${term}”` : 'Search'}</strong>
				</div>
				{#if !term}
					<p class="empty">Search for a question or a community.</p>
				{:else if listing.length === 0}
					<p class="empty">No posts match “{term}”.</p>
				{:else}
					{#each listing as p (p.id)}
						{@render card(p)}
					{/each}
				{/if}
			{:else if route.kind === 'post'}
				<div class="top-title">
					<a class="back" href="/r/{route.sub}" aria-label="Back to r/{route.sub}">
						{@render icon(P.back)}
					</a>
					<span>r/{route.sub}</span>
				</div>
				{#if postItem}
					{@render card(postItem, true)}
					<section class="thread">
						<div class="c-head">{fmt(postItem.comments)} comments</div>
						{#if hideComments}
							<div class="pad">
								<CleanedNotice label="Comments" dark />
							</div>
						{:else}
							<div class="comments">
								{#each comments as c, i (i)}
									<div
										class="comment"
										class:nested={c.depth > 0}
										style="--d: {c.depth}"
									>
										<div class="c-meta">
											<span class="c-author">u/{c.author}</span>
											<span>{c.votes} points</span>
										</div>
										<p class="c-text">{c.text}</p>
										<div class="c-actions">
											<span>Reply</span>
											<span>Share</span>
										</div>
									</div>
								{/each}
							</div>
						{/if}
					</section>
				{:else}
					<p class="empty">This post is unavailable.</p>
				{/if}
			{:else}
				<div class="top-title">
					<strong>{route.kind === 'other' ? route.name : ''}</strong>
				</div>
				<p class="empty">Nothing here yet.</p>
			{/if}
		</main>

		<aside class="right">
			{#if hideRecs}
				<CleanedNotice label="Popular communities" dark />
			{:else}
				<section class="box">
					<h2>Popular communities</h2>
					{#each subs as s, i (s.name)}
						<a class="pop" href="/r/{s.name}">
							<span class="rank">{i + 1}</span>
							{@render dot(s.name, 'lg')}
							<span class="pop-text">
								<span class="pop-name">r/{s.name}</span>
								<span class="muted">{s.members} members</span>
							</span>
						</a>
					{/each}
					<a class="more" href="/popular">View all</a>
				</section>
			{/if}
		</aside>
	</div>
</div>

<style>
	.rd {
		--line: #223237;
		--card: #0f1a1c;
		--hover: #132226;
		--muted: #8ba2ad;
		--text: #d7dadc;
		--accent: #ff4500;
		box-sizing: border-box;
		min-height: 100%;
		background: #0b1416;
		color: var(--text);
		font-family: 'IBM Plex Sans', 'Inter Variable', ui-sans-serif, sans-serif;
		font-size: 14px;
		line-height: 20px;
	}
	.rd * {
		box-sizing: border-box;
	}
	.rd a {
		color: inherit;
		text-decoration: none;
	}
	.rd button {
		font: inherit;
		color: inherit;
		cursor: pointer;
	}
	.rd p,
	.rd h1,
	.rd h2,
	.rd h3 {
		margin: 0;
	}

	/* Top bar */
	.bar {
		position: sticky;
		top: 0;
		z-index: 10;
		display: flex;
		align-items: center;
		gap: 16px;
		height: 56px;
		padding: 0 16px;
		border-bottom: 1px solid var(--line);
		background: #0b1416;
	}
	.brand {
		display: flex;
		flex: 0 0 auto;
		align-items: center;
		gap: 8px;
	}
	.mark {
		position: relative;
		width: 34px;
		height: 34px;
		border-radius: 50%;
		background: var(--accent);
	}
	.mark::after {
		content: '';
		position: absolute;
		top: 50%;
		left: 50%;
		width: 12px;
		height: 12px;
		margin: -6px 0 0 -6px;
		border-radius: 50%;
		background: #fff;
	}
	.word {
		font-size: 20px;
		font-weight: 700;
		letter-spacing: -0.01em;
	}
	.search {
		display: flex;
		flex: 1;
		align-items: center;
		gap: 8px;
		max-width: 640px;
		height: 40px;
		margin: 0 auto;
		padding: 0 14px;
		border: 1px solid transparent;
		border-radius: 999px;
		background: #1a2a2e;
		color: var(--muted);
	}
	.search:focus-within {
		border-color: var(--accent);
		background: #0b1416;
		color: var(--text);
	}
	.search input {
		flex: 1;
		min-width: 0;
		border: 0;
		outline: none;
		background: none;
		color: var(--text);
		font: inherit;
		font-size: 14px;
	}
	.search input::placeholder {
		color: var(--muted);
	}
	.login {
		flex: 0 0 auto;
		padding: 7px 18px;
		border: 0;
		border-radius: 999px;
		background: var(--accent);
		color: #fff;
		font-weight: 700;
	}
	.login:hover {
		background: #e03d00;
	}

	/* Layout */
	.body {
		display: flex;
		align-items: flex-start;
		justify-content: center;
		gap: 24px;
		max-width: 1300px;
		margin: 0 auto;
		padding: 24px 16px 40px;
	}

	.side {
		position: sticky;
		top: 72px;
		flex: 0 0 240px;
		width: 240px;
		max-height: calc(100vh - 88px);
		overflow-y: auto;
		scrollbar-width: none;
	}
	.menu {
		display: flex;
		flex-direction: column;
		gap: 2px;
		padding: 4px 0;
	}
	.menu a {
		display: flex;
		align-items: center;
		gap: 12px;
		padding: 8px 12px;
		border-radius: 8px;
		font-weight: 500;
	}
	.menu a:hover {
		background: var(--hover);
	}
	.menu a.on {
		background: var(--hover);
		font-weight: 700;
	}
	.menu .ic {
		width: 18px;
		height: 18px;
		color: var(--muted);
	}
	.label {
		padding: 14px 12px 4px;
		color: var(--muted);
		font-size: 12px;
		font-weight: 700;
		letter-spacing: 0.04em;
		text-transform: uppercase;
	}

	.center {
		flex: 0 1 720px;
		width: 720px;
		min-width: 0;
	}
	.right {
		position: sticky;
		top: 72px;
		flex: 0 0 310px;
		width: 310px;
		align-self: flex-start;
	}

	/* Sort + top title */
	.sort {
		display: flex;
		align-items: center;
		gap: 4px;
		padding: 0 0 12px;
	}
	.sort-btn {
		padding: 6px 12px;
		border: 0;
		border-radius: 999px;
		background: none;
		color: var(--muted);
		font-size: 13px;
		font-weight: 700;
	}
	.sort-btn:hover {
		background: var(--hover);
		color: var(--text);
	}
	.sort-btn.on {
		background: var(--hover);
		color: var(--text);
	}
	.top-title {
		display: flex;
		align-items: center;
		gap: 12px;
		min-height: 48px;
		padding: 4px 0 12px;
		font-size: 16px;
	}
	.top-title strong {
		font-size: 18px;
		font-weight: 700;
	}
	.back {
		display: flex;
		align-items: center;
		justify-content: center;
		width: 32px;
		height: 32px;
		border-radius: 999px;
	}
	.back:hover {
		background: var(--hover);
	}
	.back .ic {
		width: 20px;
		height: 20px;
	}

	/* Generic bits */
	.ic {
		flex: none;
		width: 18px;
		height: 18px;
		fill: none;
		stroke: currentColor;
		stroke-width: 1.8;
		stroke-linecap: round;
		stroke-linejoin: round;
	}
	.dot {
		display: grid;
		flex: none;
		place-items: center;
		width: 18px;
		height: 18px;
		border-radius: 50%;
		color: #fff;
		font-size: 10px;
		font-weight: 700;
	}
	.dot.sm {
		width: 20px;
		height: 20px;
		font-size: 11px;
	}
	.dot.lg {
		width: 28px;
		height: 28px;
		font-size: 13px;
	}
	.muted {
		color: var(--muted);
	}
	.empty {
		padding: 48px 24px;
		color: var(--muted);
		text-align: center;
	}
	.pad {
		padding: 12px 0;
	}
	.slot {
		width: 100%;
		max-width: 340px;
	}
	.calm {
		display: flex;
		flex-direction: column;
		align-items: center;
		gap: 16px;
		padding: 56px 24px;
		text-align: center;
	}
	.calm p {
		color: var(--muted);
		font-size: 16px;
		line-height: 22px;
	}

	/* Post cards */
	.post {
		position: relative;
		display: flex;
		margin-bottom: 8px;
		overflow: hidden;
		border: 1px solid var(--line);
		border-radius: 6px;
		background: var(--card);
		transition:
			border-color 0.15s ease,
			background-color 0.15s ease;
	}
	.post:hover {
		border-color: #3a5058;
		background: var(--hover);
	}
	.post.full {
		margin: 0;
		border: 0;
		border-radius: 0;
		background: none;
	}
	.post.full:hover {
		background: none;
	}
	.stretch {
		position: absolute;
		inset: 0;
	}
	.post a:not(.stretch),
	.post button {
		position: relative;
		z-index: 1;
	}
	.votes {
		display: flex;
		flex: 0 0 44px;
		flex-direction: column;
		align-items: center;
		gap: 2px;
		padding: 8px 0;
		background: rgb(0 0 0 / 0.14);
	}
	.vote {
		display: grid;
		place-items: center;
		width: 28px;
		height: 28px;
		border: 0;
		border-radius: 4px;
		background: none;
		color: var(--muted);
	}
	.vote:hover {
		background: #1d3035;
	}
	.vote.up:hover {
		color: var(--accent);
	}
	.vote.down:hover {
		color: #7193ff;
	}
	.vote .ic {
		width: 20px;
		height: 20px;
		fill: currentColor;
		stroke: none;
	}
	.score {
		color: var(--text);
		font-size: 12px;
		font-weight: 700;
	}
	.content {
		flex: 1;
		min-width: 0;
		padding: 8px 12px 6px 4px;
	}
	.line {
		display: flex;
		flex-wrap: wrap;
		align-items: center;
		gap: 4px;
		color: var(--muted);
		font-size: 12px;
	}
	.subname {
		color: var(--text);
		font-weight: 700;
	}
	.subname:hover {
		text-decoration: underline;
	}
	.sep {
		opacity: 0.6;
	}
	.title {
		margin-top: 6px;
		color: var(--text);
		font-size: 18px;
		font-weight: 500;
		line-height: 24px;
		overflow-wrap: anywhere;
	}
	.flair {
		display: inline-block;
		margin-top: 6px;
		padding: 2px 8px;
		border-radius: 999px;
		background: #1c3a44;
		color: #9fd4e6;
		font-size: 11px;
		font-weight: 700;
	}
	.preview {
		display: -webkit-box;
		margin-top: 6px;
		overflow: hidden;
		color: #b4c4ca;
		font-size: 14px;
		line-height: 20px;
		-webkit-line-clamp: 3;
		line-clamp: 3;
		-webkit-box-orient: vertical;
	}
	.media {
		position: relative;
		width: 100%;
		margin-top: 10px;
		aspect-ratio: 16 / 9;
		border: 1px solid var(--line);
		border-radius: 12px;
		overflow: hidden;
	}
	.media::after {
		content: '';
		position: absolute;
		inset: 0;
		background:
			radial-gradient(circle at 30% 20%, rgb(255 255 255 / 0.25), transparent 55%),
			linear-gradient(to top, rgb(0 0 0 / 0.3), transparent 50%);
	}
	.foot {
		display: flex;
		flex-wrap: wrap;
		gap: 4px;
		margin-top: 8px;
		margin-left: -6px;
		color: var(--muted);
		font-size: 12px;
		font-weight: 700;
	}
	.pillbtn {
		display: inline-flex;
		align-items: center;
		gap: 6px;
		padding: 6px 8px;
		border: 0;
		border-radius: 999px;
		background: none;
		color: var(--muted);
		font-size: 12px;
		font-weight: 700;
	}
	.pillbtn:hover {
		background: #1d3035;
		color: var(--text);
	}
	.foot .ic {
		width: 16px;
		height: 16px;
	}

	/* Full post (comments page) */
	.post.full .content {
		padding: 12px 0 8px 4px;
	}
	.post.full .title {
		margin-top: 8px;
		font-size: 22px;
		line-height: 28px;
	}
	.post.full .preview {
		display: block;
		overflow: visible;
		font-size: 16px;
		line-height: 24px;
	}
	.post.full .media {
		margin-top: 12px;
	}

	/* Thread */
	.thread {
		margin-top: 8px;
	}
	.c-head {
		padding: 8px 0 12px;
		color: var(--muted);
		font-size: 14px;
		font-weight: 700;
	}
	.comments {
		padding: 8px 0;
		border: 1px solid var(--line);
		border-radius: 6px;
		background: var(--card);
	}
	.comment {
		position: relative;
		padding: 10px 16px 10px calc(var(--d) * 24px + 16px);
	}
	.comment.nested::before {
		content: '';
		position: absolute;
		top: 0;
		bottom: 0;
		left: calc(var(--d) * 24px - 8px);
		width: 2px;
		border-radius: 2px;
		background: #2c4046;
	}
	.c-meta {
		display: flex;
		gap: 8px;
		color: var(--muted);
		font-size: 12px;
	}
	.c-author {
		color: var(--text);
		font-weight: 700;
	}
	.c-text {
		margin: 4px 0 6px;
		font-size: 14px;
		line-height: 20px;
	}
	.c-actions {
		display: flex;
		gap: 12px;
		color: var(--muted);
		font-size: 12px;
		font-weight: 700;
	}

	/* Sub header */
	.sub-card {
		overflow: hidden;
		margin-bottom: 12px;
		border: 1px solid var(--line);
		border-radius: 6px;
		background: var(--card);
	}
	.sub-banner {
		height: 120px;
	}
	.sub-body {
		padding: 0 16px 16px;
	}
	.sub-top {
		display: flex;
		align-items: flex-end;
		justify-content: space-between;
	}
	.sub-icon {
		display: grid;
		place-items: center;
		width: 64px;
		height: 64px;
		margin-top: -32px;
		border: 4px solid var(--card);
		border-radius: 50%;
		color: #fff;
		font-size: 26px;
		font-weight: 700;
	}
	.join {
		margin-bottom: 4px;
		padding: 7px 18px;
		border: 1px solid var(--accent);
		border-radius: 999px;
		background: none;
		color: var(--accent);
		font-weight: 700;
	}
	.join:hover {
		background: rgb(255 69 0 / 0.1);
	}
	.sub-body h1 {
		margin-top: 10px;
		font-size: 20px;
		font-weight: 700;
	}
	.members {
		color: var(--muted);
		font-size: 12px;
	}
	.about {
		margin-top: 10px;
		color: #b4c4ca;
	}

	/* Popular communities */
	.box {
		overflow: hidden;
		border: 1px solid var(--line);
		border-radius: 6px;
		background: var(--card);
	}
	.box h2 {
		padding: 12px 16px;
		border-bottom: 1px solid var(--line);
		font-size: 14px;
		font-weight: 700;
	}
	.pop {
		display: flex;
		align-items: center;
		gap: 10px;
		padding: 10px 16px;
	}
	.pop:hover {
		background: var(--hover);
	}
	.rank {
		width: 12px;
		color: var(--muted);
		font-size: 12px;
	}
	.pop-text {
		display: flex;
		flex-direction: column;
		min-width: 0;
	}
	.pop-name {
		font-weight: 700;
	}
	.more {
		display: block;
		padding: 12px 16px;
		border-top: 1px solid var(--line);
		color: var(--accent);
		font-weight: 700;
	}
	.more:hover {
		background: var(--hover);
	}

	@media (max-width: 1180px) {
		.right {
			display: none;
		}
	}
	@media (max-width: 900px) {
		.side {
			display: none;
		}
	}
</style>
