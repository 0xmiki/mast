<script lang="ts">
	import type { SiteProps } from './types';
	import { gradientFor, hash } from './types';
	import CleanedNotice from './CleanedNotice.svelte';
	import { posts, trends, suggested, replyPool, bios, me } from './x-data';
	import type { Post } from './x-data';

	let { url, clean, navigate }: SiteProps = $props();

	/* Icon paths (24px grid, stroked). */
	const P = {
		home: 'M3 10.5 12 3l9 7.5V21h-6v-6H9v6H3z',
		search: 'M10.5 3.5a7 7 0 1 0 0 14 7 7 0 0 0 0-14zM20.5 20.5l-4.6-4.6',
		bell: 'M6 8a6 6 0 0 1 12 0c0 6 2.5 7.5 2.5 7.5h-17S6 14 6 8zM10 19a2 2 0 0 0 4 0',
		mail: 'M3 5h18v14H3zM3 6l9 7 9-7',
		bookmark: 'M6 3h12v18l-6-4.5L6 21z',
		user: 'M12 12a4 4 0 1 0 0-8 4 4 0 0 0 0 8zM4 21c0-4.4 3.6-7 8-7s8 2.6 8 7',
		reply: 'M4 5h16v11H9l-5 4z',
		repost: 'M17 2l4 4-4 4M3 11V9a3 3 0 0 1 3-3h15M7 22l-4-4 4-4M21 13v2a3 3 0 0 1-3 3H3',
		heart:
			'M12 20.5s-7.5-4.6-9.2-9.1C1.7 8.5 3.6 5 7.2 5c2 0 3.5 1.1 4.8 2.8C13.3 6.1 14.8 5 16.8 5c3.6 0 5.5 3.5 4.4 6.4C19.5 15.9 12 20.5 12 20.5z',
		views: 'M4 20V12M10 20V6M16 20v-8M21 20H3',
		back: 'M15 5l-7 7 7 7'
	};

	const OTHER_PAGES = ['explore', 'notifications', 'messages', 'bookmarks'];

	type Route =
		| { kind: 'home' }
		| { kind: 'search' }
		| { kind: 'other'; name: string }
		| { kind: 'profile'; handle: string }
		| { kind: 'status'; handle: string; id: string };

	function safeDecode(s: string): string {
		try {
			return decodeURIComponent(s);
		} catch {
			return s;
		}
	}

	const parts = $derived(url.pathname.split('/').filter(Boolean).map(safeDecode));

	const route = $derived.by((): Route => {
		const [first, second, third] = parts;
		if (!first) return { kind: 'home' };
		if (first === 'search') return { kind: 'search' };
		if (OTHER_PAGES.includes(first)) return { kind: 'other', name: first };
		if (second === 'status' && third) return { kind: 'status', handle: first, id: third };
		return { kind: 'profile', handle: first };
	});

	const term = $derived((url.searchParams.get('q') ?? '').trim());

	const hideFeed = $derived(clean.includes('feed'));
	const hideRecs = $derived(clean.includes('recommendations'));
	const hideComments = $derived(clean.includes('comments'));

	const searchResults = $derived(
		term ? posts.filter((p) => p.text.toLowerCase().includes(term.toLowerCase())) : []
	);

	const profile = $derived.by(() => {
		if (route.kind !== 'profile') return null;
		const handle = route.handle;
		const matches = posts.filter((p) => p.handle.toLowerCase() === handle.toLowerCase());
		return {
			handle,
			name: matches.length ? matches[0].name : handle,
			bio: bios[hash(handle) % bios.length],
			following: 40 + (hash(handle + ':following') % 900),
			followers: 1000 + (hash(handle + ':followers') % 480000),
			list: matches.length ? matches : posts.map((p) => ({ ...p, handle }))
		};
	});

	const statusPost = $derived.by((): Post | null => {
		if (route.kind !== 'status') return null;
		const id = route.id;
		return posts.find((p) => p.id === id) ?? null;
	});

	const replies = $derived.by((): Post[] => {
		if (!statusPost) return [];
		const start = hash(statusPost.id) % replyPool.length;
		return Array.from({ length: 5 }, (_, i) => replyPool[(start + i) % replyPool.length]);
	});

	function fmt(n: number): string {
		const trim = (x: number) => (x >= 100 ? x.toFixed(0) : x.toFixed(1)).replace(/\.0$/, '');
		if (n >= 1_000_000) return trim(n / 1_000_000) + 'M';
		if (n >= 1_000) return trim(n / 1_000) + 'K';
		return String(n);
	}

	function initials(name: string): string {
		return name
			.split(/\s+/)
			.filter(Boolean)
			.slice(0, 2)
			.map((w) => w[0])
			.join('')
			.toUpperCase();
	}

	/** Splits post text so #tags and @mentions can be tinted. */
	function segment(text: string) {
		return text.split(/([#@]\w+)/).map((t) => ({ t, tag: /^[#@]\w+$/.test(t) }));
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

{#snippet avatar(name: string, seed: string, big: boolean = false)}
	<div class="avatar" class:lg={big} style="background: {gradientFor(seed)}" aria-hidden="true">
		{initials(name)}
	</div>
{/snippet}

{#snippet composer(prompt: string, cta: string)}
	<div class="composer">
		{@render avatar(me.name, me.handle)}
		<div class="compose-main">
			<div class="ph">{prompt}</div>
			<div class="compose-foot">
				<button type="button" class="pill" disabled>{cta}</button>
			</div>
		</div>
	</div>
{/snippet}

{#snippet actions(p: Post)}
	<div class="actions">
		<button type="button" class="act" aria-label="Reply">
			{@render icon(P.reply)}<span>{fmt(p.replies)}</span>
		</button>
		<button type="button" class="act rt" aria-label="Repost">
			{@render icon(P.repost)}<span>{fmt(p.reposts)}</span>
		</button>
		<button type="button" class="act lk" aria-label="Like">
			{@render icon(P.heart)}<span>{fmt(p.likes)}</span>
		</button>
		<span class="views" aria-label="Views">
			{@render icon(P.views)}<span>{p.views}</span>
		</span>
	</div>
{/snippet}

{#snippet card(p: Post, replyTo: string | null = null)}
	<article class="post">
		{#if replyTo === null}
			<a class="stretch" href="/{p.handle}/status/{p.id}" aria-label="Open post by {p.name}"></a>
		{/if}
		<a class="avatar-link" href="/{p.handle}">{@render avatar(p.name, p.handle)}</a>
		<div class="body">
			<div class="meta">
				<a class="name" href="/{p.handle}">{p.name}</a>
				<span class="muted">@{p.handle} · {p.age}</span>
			</div>
			{#if replyTo !== null}
				<div class="replying">Replying to <a href="/{replyTo}">@{replyTo}</a></div>
			{/if}
			<p class="text">{#each segment(p.text) as s, i (i)}{#if s.tag}<span class="tag">{s.t}</span>{:else}{s.t}{/if}{/each}</p>
			{#if p.media}
				<div class="media" style="background: {gradientFor(p.id)}"></div>
			{/if}
			{@render actions(p)}
		</div>
	</article>
{/snippet}

<div class="x">
	<div class="frame">
		<aside class="left">
			<nav class="nav" aria-label="Primary">
				<a class="logo" href="/" aria-label="X home">X</a>
				<a class="nav-item" class:active={route.kind === 'home'} href="/">
					{@render icon(P.home)}<span>Home</span>
				</a>
				<a class="nav-item" href="/explore">
					{@render icon(P.search)}<span>Explore</span>
				</a>
				<a class="nav-item" href="/notifications">
					{@render icon(P.bell)}<span>Notifications</span><b class="badge">20+</b>
				</a>
				<a class="nav-item" href="/messages">
					{@render icon(P.mail)}<span>Messages</span>
				</a>
				<a class="nav-item" href="/bookmarks">
					{@render icon(P.bookmark)}<span>Bookmarks</span>
				</a>
				<a
					class="nav-item"
					class:active={route.kind === 'profile' && route.handle === me.handle}
					href="/{me.handle}"
				>
					{@render icon(P.user)}<span>Profile</span>
				</a>
				<button type="button" class="post-btn">Post</button>
			</nav>
		</aside>

		<main class="center">
			{#if route.kind === 'home'}
				<header class="top">
					<div class="tabs" role="tablist">
						<button type="button" class="tab on" role="tab" aria-selected="true">For you</button>
						<button type="button" class="tab" role="tab" aria-selected="false">Following</button>
					</div>
				</header>
				{#if hideFeed}
					<div class="calm">
						<p>Your timeline is hidden. Search or visit a profile.</p>
						<div class="slot">
							<CleanedNotice label="For you feed" dark />
						</div>
					</div>
				{:else}
					{@render composer('What is happening?!', 'Post')}
					{#each posts as p (p.id)}
						{@render card(p)}
					{/each}
				{/if}
			{:else if route.kind === 'search'}
				<header class="top">
					<div class="top-title">
						<strong>{term ? `Results for “${term}”` : 'Search'}</strong>
					</div>
				</header>
				{#if !term}
					<p class="empty">Search for a word or phrase to see matching posts.</p>
				{:else if searchResults.length === 0}
					<p class="empty">No posts match “{term}”.</p>
				{:else}
					{#each searchResults as p (p.id)}
						{@render card(p)}
					{/each}
				{/if}
			{:else if route.kind === 'other'}
				<header class="top">
					<div class="top-title">
						<strong>{route.name.charAt(0).toUpperCase() + route.name.slice(1)}</strong>
					</div>
				</header>
				<p class="empty">Nothing here yet.</p>
			{:else if route.kind === 'profile' && profile}
				<header class="top">
					<div class="top-title">
						<a class="back" href="/" aria-label="Back to home">{@render icon(P.back)}</a>
						<div>
							<strong>{profile.name}</strong>
							<span class="muted small">{profile.list.length} posts</span>
						</div>
					</div>
				</header>
				<div class="banner" style="background: {gradientFor(profile.handle)}"></div>
				<div class="p-head">
					<div class="avatar xl" style="background: {gradientFor(profile.handle)}" aria-hidden="true">
						{initials(profile.name)}
					</div>
					<button type="button" class="pill-outline">Follow</button>
				</div>
				<div class="p-body">
					<h1>{profile.name}</h1>
					<div class="muted">@{profile.handle}</div>
					<p class="bio">{profile.bio}</p>
					<div class="counts">
						<span><b>{fmt(profile.following)}</b> Following</span>
						<span><b>{fmt(profile.followers)}</b> Followers</span>
					</div>
				</div>
				{#each profile.list as p (p.id)}
					{@render card(p)}
				{/each}
			{:else if route.kind === 'status'}
				<header class="top">
					<div class="top-title">
						<a class="back" href="/" aria-label="Back to home">{@render icon(P.back)}</a>
						<strong>Post</strong>
					</div>
				</header>
				{#if statusPost}
					<article class="detail">
						<div class="d-head">
							{@render avatar(statusPost.name, statusPost.handle, true)}
							<div>
								<a class="name" href="/{statusPost.handle}">{statusPost.name}</a>
								<div class="muted">@{statusPost.handle}</div>
							</div>
						</div>
						<p class="big">{#each segment(statusPost.text) as s, i (i)}{#if s.tag}<span class="tag">{s.t}</span>{:else}{s.t}{/if}{/each}</p>
						{#if statusPost.media}
							<div class="media" style="background: {gradientFor(statusPost.id)}"></div>
						{/if}
						<div class="stamp">{statusPost.age} · {statusPost.views} views</div>
						<div class="counts">
							<span><b>{fmt(statusPost.reposts)}</b> Reposts</span>
							<span><b>{fmt(statusPost.likes)}</b> Likes</span>
						</div>
						{@render actions(statusPost)}
					</article>
					{@render composer('Post your reply', 'Reply')}
					{#if hideComments}
						<div class="pad">
							<CleanedNotice label="Replies" dark />
						</div>
					{:else}
						{#each replies as r (r.id)}
							{@render card(r, statusPost.handle)}
						{/each}
					{/if}
				{:else}
					<p class="empty">This post is unavailable.</p>
				{/if}
			{/if}
		</main>

		<aside class="right">
			<form class="search" role="search" onsubmit={submitSearch}>
				{@render icon(P.search)}
				<input
					name="q"
					type="search"
					placeholder="Search"
					aria-label="Search"
					autocomplete="off"
					value={term}
				/>
			</form>

			{#if hideRecs}
				<div class="slot">
					<CleanedNotice label="Trends" dark />
				</div>
			{:else}
				<section class="card">
					<h2>What’s happening</h2>
					{#each trends as t (t.topic)}
						<a class="trend" href="/search?q={encodeURIComponent(t.topic)}">
							<div class="t-cat">{t.category}</div>
							<div class="t-topic">{t.topic}</div>
							<div class="t-posts">{t.posts}</div>
						</a>
					{/each}
					<a class="more" href="/explore">Show more</a>
				</section>
			{/if}

			{#if hideRecs}
				<div class="slot">
					<CleanedNotice label="Who to follow" dark />
				</div>
			{:else}
				<section class="card">
					<h2>Who to follow</h2>
					{#each suggested as s (s.handle)}
						<div class="sug">
							<a class="sug-who" href="/{s.handle}">
								{@render avatar(s.name, s.handle)}
								<div class="sug-text">
									<div class="sug-name">{s.name}</div>
									<div class="muted">@{s.handle}</div>
								</div>
							</a>
							<button type="button" class="white-pill">Follow</button>
						</div>
					{/each}
					<a class="more" href="/explore">Show more</a>
				</section>
			{/if}
		</aside>
	</div>
</div>

<style>
	.x {
		--text: #e7e9ea;
		--muted: #71767b;
		--line: #2f3336;
		--accent: #1d9bf0;
		--hover: rgb(231 233 234 / 0.03);
		box-sizing: border-box;
		display: flex;
		justify-content: center;
		min-height: 100%;
		background: #000;
		color: var(--text);
		font-family: -apple-system, 'Segoe UI', 'Inter Variable', ui-sans-serif, sans-serif;
		font-size: 15px;
		line-height: 20px;
	}
	.x * {
		box-sizing: border-box;
	}
	.x a {
		color: inherit;
		text-decoration: none;
	}
	.x button {
		font: inherit;
		color: inherit;
		cursor: pointer;
	}
	.x p,
	.x h1,
	.x h2 {
		margin: 0;
	}

	.frame {
		display: flex;
		justify-content: center;
		width: 100%;
		max-width: 1280px;
		min-height: 100%;
	}

	/* Left navigation */
	.left {
		flex: 0 0 260px;
		display: flex;
		justify-content: flex-end;
	}
	.nav {
		position: sticky;
		top: 0;
		display: flex;
		flex-direction: column;
		align-items: flex-start;
		gap: 2px;
		width: 260px;
		height: 100vh;
		padding: 4px 16px 16px 0;
	}
	.logo {
		display: flex;
		align-items: center;
		justify-content: center;
		width: 52px;
		height: 52px;
		margin: 0 0 6px 6px;
		border-radius: 999px;
		font-size: 30px;
		font-weight: 800;
		letter-spacing: -0.02em;
	}
	.logo:hover {
		background: rgb(231 233 234 / 0.1);
	}
	.nav-item {
		display: flex;
		align-items: center;
		gap: 20px;
		width: fit-content;
		padding: 12px 18px 12px 14px;
		border-radius: 999px;
		font-size: 20px;
		line-height: 24px;
	}
	.nav-item:hover {
		background: rgb(231 233 234 / 0.1);
	}
	.nav-item.active {
		font-weight: 700;
	}
	.nav-item .ic {
		width: 26px;
		height: 26px;
	}
	.badge {
		margin-left: 4px;
		padding: 1px 7px;
		border-radius: 999px;
		background: var(--accent);
		color: #fff;
		font-size: 13px;
		font-weight: 700;
		line-height: 18px;
	}
	.post-btn {
		width: 100%;
		margin-top: 16px;
		padding: 15px 0;
		border: 0;
		border-radius: 999px;
		background: var(--accent);
		color: #fff;
		font-size: 17px;
		font-weight: 700;
	}
	.post-btn:hover {
		background: #1a8cd8;
	}

	/* Centre timeline */
	.center {
		flex: 0 1 600px;
		width: 600px;
		min-width: 0;
		min-height: 100%;
		border-left: 1px solid var(--line);
		border-right: 1px solid var(--line);
	}
	.top {
		position: sticky;
		top: 0;
		z-index: 5;
		background: rgb(0 0 0 / 0.65);
		backdrop-filter: blur(12px);
		border-bottom: 1px solid var(--line);
	}
	.top-title {
		display: flex;
		align-items: center;
		gap: 20px;
		min-height: 53px;
		padding: 8px 16px;
	}
	.top-title strong {
		font-size: 20px;
		font-weight: 800;
		line-height: 24px;
	}
	.top-title .muted {
		display: block;
		font-size: 13px;
	}
	.back {
		display: flex;
		align-items: center;
		justify-content: center;
		width: 34px;
		height: 34px;
		border-radius: 999px;
	}
	.back:hover {
		background: rgb(231 233 234 / 0.1);
	}
	.back .ic {
		width: 20px;
		height: 20px;
	}
	.tabs {
		display: flex;
	}
	.tab {
		position: relative;
		flex: 1;
		padding: 16px 0;
		border: 0;
		background: none;
		color: var(--muted);
		font-size: 15px;
		font-weight: 600;
	}
	.tab:hover {
		background: var(--hover);
	}
	.tab.on {
		color: var(--text);
	}
	.tab.on::after {
		content: '';
		position: absolute;
		bottom: 0;
		left: 50%;
		width: 56px;
		height: 4px;
		border-radius: 999px;
		background: var(--accent);
		transform: translateX(-50%);
	}

	.composer {
		display: flex;
		gap: 12px;
		padding: 12px 16px;
		border-bottom: 1px solid var(--line);
	}
	.compose-main {
		display: flex;
		flex: 1;
		flex-direction: column;
		min-width: 0;
	}
	.ph {
		padding: 12px 0;
		color: var(--muted);
		font-size: 20px;
	}
	.compose-foot {
		display: flex;
		justify-content: flex-end;
		padding-top: 8px;
	}
	.pill {
		padding: 8px 18px;
		border: 0;
		border-radius: 999px;
		background: var(--accent);
		color: #fff;
		font-weight: 700;
		opacity: 0.5;
	}

	.avatar {
		display: grid;
		flex: none;
		place-items: center;
		width: 40px;
		height: 40px;
		border-radius: 50%;
		color: rgb(255 255 255 / 0.95);
		font-size: 15px;
		font-weight: 700;
		letter-spacing: 0.02em;
		text-shadow: 0 1px 2px rgb(0 0 0 / 0.25);
	}
	.avatar.lg {
		width: 56px;
		height: 56px;
		font-size: 20px;
	}
	.avatar.xl {
		width: 134px;
		height: 134px;
		border: 4px solid #000;
		font-size: 48px;
	}

	.post {
		position: relative;
		display: flex;
		gap: 12px;
		padding: 12px 16px;
		border-bottom: 1px solid var(--line);
		transition: background-color 0.15s ease;
	}
	.post:hover {
		background-color: var(--hover);
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
	.avatar-link {
		display: flex;
	}
	.body {
		flex: 1;
		min-width: 0;
	}
	.meta {
		display: flex;
		flex-wrap: wrap;
		align-items: baseline;
		gap: 0 4px;
	}
	.name {
		font-weight: 700;
		color: var(--text);
	}
	.name:hover {
		text-decoration: underline;
	}
	.muted {
		color: var(--muted);
	}
	.small {
		font-size: 13px;
	}
	.replying {
		margin-top: 2px;
		color: var(--muted);
	}
	.replying a {
		color: var(--accent);
	}
	.text {
		margin-top: 2px;
		white-space: pre-line;
		overflow-wrap: anywhere;
	}
	.tag {
		color: var(--accent);
	}
	.media {
		position: relative;
		width: 100%;
		margin-top: 12px;
		aspect-ratio: 16 / 9;
		border: 1px solid var(--line);
		border-radius: 16px;
		overflow: hidden;
	}
	.media::after {
		content: '';
		position: absolute;
		inset: 0;
		background:
			radial-gradient(circle at 30% 20%, rgb(255 255 255 / 0.28), transparent 55%),
			linear-gradient(to top, rgb(0 0 0 / 0.25), transparent 50%);
	}

	.actions {
		display: flex;
		justify-content: space-between;
		max-width: 425px;
		margin-top: 8px;
		color: var(--muted);
	}
	.act,
	.views {
		display: inline-flex;
		align-items: center;
		gap: 8px;
		padding: 6px 4px;
		border: 0;
		border-radius: 999px;
		background: none;
		color: inherit;
		font-size: 13px;
	}
	.act:hover {
		color: var(--accent);
		background: rgb(29 155 240 / 0.1);
	}
	.act.rt:hover {
		color: #00ba7c;
		background: rgb(0 186 124 / 0.1);
	}
	.act.lk:hover {
		color: #f91880;
		background: rgb(249 24 128 / 0.1);
	}
	.views {
		cursor: default;
	}
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

	/* Profile */
	.banner {
		height: 200px;
	}
	.p-head {
		display: flex;
		align-items: flex-end;
		justify-content: space-between;
		padding: 0 16px;
	}
	.p-head .avatar {
		margin-top: -67px;
	}
	.pill-outline {
		margin-bottom: 12px;
		padding: 8px 16px;
		border: 1px solid var(--muted);
		border-radius: 999px;
		background: none;
		font-size: 15px;
		font-weight: 700;
	}
	.pill-outline:hover {
		background: rgb(231 233 234 / 0.1);
	}
	.p-body {
		padding: 12px 16px 16px;
		border-bottom: 1px solid var(--line);
	}
	.p-body h1 {
		font-size: 20px;
		font-weight: 800;
		line-height: 24px;
	}
	.bio {
		margin-top: 12px;
	}
	.p-body .counts {
		padding: 12px 0 0;
		border: 0;
	}
	.counts {
		display: flex;
		gap: 20px;
		color: var(--muted);
	}
	.counts b {
		color: var(--text);
	}

	/* Status page */
	.detail {
		padding: 12px 16px 0;
		border-bottom: 1px solid var(--line);
	}
	.d-head {
		display: flex;
		align-items: center;
		gap: 12px;
	}
	.d-head .name {
		display: block;
		font-size: 16px;
	}
	.big {
		margin-top: 16px;
		font-size: 23px;
		line-height: 30px;
		white-space: pre-line;
		overflow-wrap: anywhere;
	}
	.stamp {
		padding: 16px 0;
		border-bottom: 1px solid var(--line);
		color: var(--muted);
	}
	.detail .counts {
		padding: 14px 0;
		border-bottom: 1px solid var(--line);
	}
	.detail .actions {
		max-width: none;
		justify-content: space-around;
		padding: 4px 0 6px;
	}
	.pad {
		padding: 16px;
	}

	.calm {
		display: flex;
		flex-direction: column;
		align-items: center;
		gap: 16px;
		padding: 72px 40px;
		text-align: center;
	}
	.calm p {
		max-width: 340px;
		color: var(--muted);
		font-size: 18px;
		line-height: 24px;
	}
	.calm .slot {
		width: 100%;
		max-width: 340px;
		margin-top: 8px;
	}
	.empty {
		padding: 48px 32px;
		color: var(--muted);
		text-align: center;
	}

	/* Right column */
	.right {
		position: sticky;
		top: 0;
		flex: 0 0 350px;
		align-self: flex-start;
		width: 350px;
		max-height: 100vh;
		padding: 0 0 16px 24px;
		overflow-y: auto;
		scrollbar-width: none;
	}
	.search {
		display: flex;
		align-items: center;
		gap: 12px;
		height: 44px;
		margin-top: 4px;
		padding: 0 16px;
		border: 1px solid transparent;
		border-radius: 999px;
		background: #202327;
		color: var(--muted);
	}
	.search:focus-within {
		border-color: var(--accent);
		background: #000;
		color: var(--accent);
	}
	.search input {
		flex: 1;
		min-width: 0;
		border: 0;
		outline: none;
		background: none;
		color: var(--text);
		font: inherit;
		font-size: 15px;
	}
	.search input::placeholder {
		color: var(--muted);
	}
	.search .ic {
		width: 18px;
		height: 18px;
	}
	.card {
		margin-top: 16px;
		overflow: hidden;
		border-radius: 16px;
		background: #16181c;
	}
	.card h2 {
		padding: 12px 16px;
		font-size: 20px;
		font-weight: 800;
		line-height: 24px;
	}
	.trend,
	.more,
	.sug {
		display: block;
		padding: 12px 16px;
	}
	.sug {
		display: flex;
		align-items: center;
		gap: 12px;
	}
	.trend:hover,
	.more:hover,
	.sug:hover {
		background: var(--hover);
	}
	.t-cat {
		color: var(--muted);
		font-size: 13px;
	}
	.t-topic {
		margin-top: 2px;
		font-weight: 700;
	}
	.t-posts {
		margin-top: 2px;
		color: var(--muted);
		font-size: 13px;
	}
	.more {
		color: var(--accent);
	}
	.sug-who {
		display: flex;
		flex: 1;
		align-items: center;
		gap: 12px;
		min-width: 0;
	}
	.sug-text {
		min-width: 0;
		line-height: 18px;
	}
	.sug-name {
		overflow: hidden;
		font-weight: 700;
		text-overflow: ellipsis;
		white-space: nowrap;
	}
	.white-pill {
		padding: 7px 16px;
		border: 0;
		border-radius: 999px;
		background: var(--text);
		color: #0f1419;
		font-size: 14px;
		font-weight: 700;
	}
	.white-pill:hover {
		background: #d7dbdc;
	}
	.slot {
		margin-top: 16px;
	}

	@media (max-width: 1100px) {
		.right {
			display: none;
		}
	}
	@media (max-width: 900px) {
		.nav {
			width: 84px;
		}
		.nav-item span,
		.nav-item .badge,
		.post-btn {
			display: none;
		}
	}
</style>
