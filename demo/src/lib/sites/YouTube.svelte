<script lang="ts">
	import type { SiteProps } from './types';
	import { gradientFor, hash } from './types';
	import CleanedNotice from './CleanedNotice.svelte';
	import {
		videos,
		shorts,
		comments,
		getVideo,
		getShort,
		descriptionFor,
		searchVideos
	} from './youtube-data';
	import type { Video } from './youtube-data';

	let { url, clean, navigate }: SiteProps = $props();

	const path = $derived(url.pathname);
	const route: 'watch' | 'shorts' | 'results' | 'home' = $derived(
		path === '/watch'
			? 'watch'
			: path.startsWith('/shorts/')
				? 'shorts'
				: path === '/results'
					? 'results'
					: 'home'
	);
	const q = $derived(url.searchParams.get('search_query') ?? '');

	const current = $derived(getVideo(url.searchParams.get('v') ?? ''));
	const upNext = $derived(videos.filter((v) => v.id !== current.id).slice(0, 10));
	const likes = $derived(`${(hash(current.id) % 90) + 4}K`);

	const currentShort = $derived(getShort(path.slice('/shorts/'.length).split('/')[0]));
	const shortLikes = $derived(`${(hash(currentShort.id) % 400) + 12}K`);
	const shortReplies = $derived(`${(hash(currentShort.id + 'c') % 90) + 3}K`);

	const found = $derived(searchVideos(q));

	let query = $state('');
	$effect(() => {
		query = q;
	});

	function onSearch(e: SubmitEvent) { e.preventDefault(); const term = query.trim(); if (term) navigate('/results?search_query=' + encodeURIComponent(term)); }
</script>

<div class="yt">
	<header class="top">
		<div class="top-left">
			<button class="icon-btn" type="button" aria-label="Guide">
				<svg viewBox="0 0 24 24" width="24" height="24" aria-hidden="true">
					<path d="M3 6h18M3 12h18M3 18h18" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" />
				</svg>
			</button>
			<a class="brand" href="/">
				<span class="badge">
					<svg viewBox="0 0 24 24" width="12" height="12" aria-hidden="true"><path d="M8 5v14l11-7z" fill="#fff" /></svg>
				</span>
				<span class="wordmark">YouTube</span>
			</a>
		</div>
		{@render searchForm('bar')}
		<div class="top-right">
			<div class="avatar" style="background: {gradientFor('mast-user')}">M</div>
		</div>
	</header>

	{#if route === 'watch'}
		<div class="watch">
			<div class="watch-main">
				<div class="player" style="background: {gradientFor(current.id)}">
					<button class="big-play" type="button" aria-label="Play">
						<svg viewBox="0 0 24 24" width="34" height="34" aria-hidden="true"><path d="M8 5.5v13l10.5-6.5z" fill="#fff" /></svg>
					</button>
					<div class="controls">
						<span class="time">4:12 / {current.duration}</span>
						<div class="scrub"><div class="played" style="width: 35%"></div></div>
					</div>
				</div>

				<h1 class="w-title">{current.title}</h1>

				<div class="w-row">
					<div class="chan">
						<div class="avatar lg" style="background: {gradientFor(current.channel)}">{current.channel.charAt(0)}</div>
						<div class="chan-text">
							<div class="chan-name">{current.channel}</div>
							<div class="chan-subs">1.2M subscribers</div>
						</div>
						<button class="subscribe" type="button">Subscribe</button>
					</div>
					<div class="actions">
						<button class="pill" type="button">
							<svg viewBox="0 0 24 24" width="20" height="20" aria-hidden="true"><path d="M7 11v9H4v-9zM7 11l4-7c1.5 0 2.5 1 2.5 2.5L13 10h6.5a2 2 0 0 1 2 2.3l-1.3 7A2 2 0 0 1 18.2 21H7" fill="none" stroke="currentColor" stroke-width="1.8" stroke-linejoin="round" /></svg>
							<span>{likes}</span>
						</button>
						<button class="pill" type="button">
							<svg viewBox="0 0 24 24" width="20" height="20" aria-hidden="true"><path d="M15 5l6 6-6 6M21 11H10a6 6 0 0 0-6 6v2" fill="none" stroke="currentColor" stroke-width="1.8" stroke-linecap="round" stroke-linejoin="round" /></svg>
							<span>Share</span>
						</button>
					</div>
				</div>

				<div class="desc">
					<div class="desc-stats">{current.views} • {current.age}</div>
					<p>{descriptionFor(current)}</p>
				</div>

				{#if clean.includes('comments')}
					<div class="block"><CleanedNotice label="Comments" dark /></div>
				{:else}
					<section class="comments">
						<div class="c-head">
							<h2>1,284 Comments</h2>
							<button class="sort" type="button">
								<svg viewBox="0 0 24 24" width="18" height="18" aria-hidden="true"><path d="M3 6h12M3 12h8M3 18h4" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" /></svg>
								<span>Sort by</span>
							</button>
						</div>
						<div class="c-add">
							<div class="avatar sm" style="background: {gradientFor('mast-user')}">M</div>
							<span class="c-placeholder">Add a comment...</span>
						</div>
						{#each comments as c (c.handle)}
							<div class="comment">
								<div class="avatar" style="background: {gradientFor(c.handle)}">{c.handle.charAt(1).toUpperCase()}</div>
								<div class="c-body">
									<div class="c-meta"><span class="c-handle">{c.handle}</span><span class="c-age">{c.age}</span></div>
									<p>{c.text}</p>
									<div class="c-like">
										<svg viewBox="0 0 24 24" width="16" height="16" aria-hidden="true"><path d="M7 11v9H4v-9zM7 11l4-7c1.5 0 2.5 1 2.5 2.5L13 10h6.5a2 2 0 0 1 2 2.3l-1.3 7A2 2 0 0 1 18.2 21H7" fill="none" stroke="currentColor" stroke-width="1.8" stroke-linejoin="round" /></svg>
										<span>{c.likes}</span>
									</div>
								</div>
							</div>
						{/each}
					</section>
				{/if}
			</div>

			<aside class="up-next">
				{#if clean.includes('recommendations')}
					<CleanedNotice label="Recommendations" dark />
				{:else}
					<h2 class="up-head">Up next</h2>
					{#each upNext as v (v.id)}
						{@render compact(v)}
					{/each}
				{/if}
			</aside>
		</div>
	{:else if route === 'shorts'}
		<div class="shorts-page">
			{#if clean.includes('shorts')}
				<div class="centered">
					<div class="notice-wrap"><CleanedNotice label="Shorts" dark /></div>
					<a class="back" href="/">Back to home</a>
				</div>
			{:else}
				<div class="short-stage">
					<div class="short-card" style="background: {gradientFor(currentShort.id)}">
						<svg class="hint" viewBox="0 0 24 24" width="64" height="64" aria-hidden="true"><path d="M8 5.5v13l10.5-6.5z" fill="#fff" /></svg>
						<div class="short-overlay">
							<div class="s-title">{currentShort.title}</div>
							<div class="s-views">{currentShort.views} views</div>
						</div>
					</div>
					<div class="short-actions">
						<button class="s-act" type="button" aria-label="Like">
							<span class="s-circle">
								<svg viewBox="0 0 24 24" width="24" height="24" aria-hidden="true"><path d="M7 11v9H4v-9zM7 11l4-7c1.5 0 2.5 1 2.5 2.5L13 10h6.5a2 2 0 0 1 2 2.3l-1.3 7A2 2 0 0 1 18.2 21H7" fill="none" stroke="currentColor" stroke-width="1.8" stroke-linejoin="round" /></svg>
							</span>
							<span class="s-count">{shortLikes}</span>
						</button>
						<button class="s-act" type="button" aria-label="Comments">
							<span class="s-circle">
								<svg viewBox="0 0 24 24" width="24" height="24" aria-hidden="true"><path d="M4 5h16v11H9l-5 4z" fill="none" stroke="currentColor" stroke-width="1.8" stroke-linejoin="round" /></svg>
							</span>
							<span class="s-count">{shortReplies}</span>
						</button>
						<button class="s-act" type="button" aria-label="Share">
							<span class="s-circle">
								<svg viewBox="0 0 24 24" width="24" height="24" aria-hidden="true"><path d="M15 5l6 6-6 6M21 11H10a6 6 0 0 0-6 6v2" fill="none" stroke="currentColor" stroke-width="1.8" stroke-linecap="round" stroke-linejoin="round" /></svg>
							</span>
							<span class="s-count">Share</span>
						</button>
					</div>
				</div>
			{/if}
		</div>
	{:else if route === 'results'}
		<div class="results">
			<h1 class="r-head">{q ? `Results for “${q}”` : 'Results'}</h1>
			{#each found as v (v.id)}
				<a class="result" href="/watch?v={v.id}">
					<div class="thumb r-thumb" style="background: {gradientFor(v.id)}">
						<span class="dur">{v.duration}</span>
					</div>
					<div class="r-text">
						<h3 class="r-title">{v.title}</h3>
						<div class="r-meta">{v.views} • {v.age}</div>
						<div class="r-chan">
							<span class="avatar sm" style="background: {gradientFor(v.channel)}">{v.channel.charAt(0)}</span>
							<span>{v.channel}</span>
						</div>
						<p class="r-desc">{descriptionFor(v)}</p>
					</div>
				</a>
			{/each}
		</div>
	{:else}
		<div class="home">
			<aside class="side" aria-label="Guide">
				<a class="nav on" href="/">
					<svg viewBox="0 0 24 24" width="24" height="24" aria-hidden="true"><path d="M3 10.5 12 3l9 7.5V21h-6v-6H9v6H3z" fill="currentColor" /></svg>
					<span>Home</span>
				</a>
				{#if !clean.includes('shorts')}
					<a class="nav" href="/shorts/{shorts[0].id}">
						<svg viewBox="0 0 24 24" width="24" height="24" aria-hidden="true"><rect x="5" y="3" width="14" height="18" rx="4" fill="none" stroke="currentColor" stroke-width="2" /><path d="M10 9.5l5 2.5-5 2.5z" fill="currentColor" /></svg>
						<span>Shorts</span>
					</a>
				{/if}
				<a class="nav" href="/feed/subscriptions">
					<svg viewBox="0 0 24 24" width="24" height="24" aria-hidden="true"><rect x="3" y="6" width="18" height="12" rx="2" fill="none" stroke="currentColor" stroke-width="2" /><path d="M3 3h18" stroke="currentColor" stroke-width="2" stroke-linecap="round" /></svg>
					<span>Subscriptions</span>
				</a>
				<a class="nav" href="/feed/you">
					<svg viewBox="0 0 24 24" width="24" height="24" aria-hidden="true"><circle cx="12" cy="8" r="4" fill="none" stroke="currentColor" stroke-width="2" /><path d="M4 21c1-4 4-6 8-6s7 2 8 6" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" /></svg>
					<span>You</span>
				</a>
			</aside>

			<main class="main">
				{#if clean.includes('feed')}
					<section class="calm">
						{@render searchForm('hero')}
						<p class="calm-text">Search for what you came for.</p>
						<div class="notice-wrap"><CleanedNotice label="Home feed" dark /></div>
					</section>
				{:else}
					<div class="chips">
						{#each ['All', 'Music', 'Gaming', 'Podcasts', 'Live', 'Cooking', 'Coding', 'Recently uploaded'] as chip, i (chip)}
							<button class="chip" class:on={i === 0} type="button">{chip}</button>
						{/each}
					</div>

					<div class="grid">
						{#each videos.slice(0, 8) as v (v.id)}
							{@render card(v)}
						{/each}
					</div>

					{#if clean.includes('shorts')}
						<div class="notice-row"><CleanedNotice label="Shorts" dark /></div>
					{:else}
						<section class="shelf">
							<div class="shelf-head">
								<svg viewBox="0 0 24 24" width="22" height="22" aria-hidden="true"><rect x="5" y="3" width="14" height="18" rx="4" fill="none" stroke="#ff3b3b" stroke-width="2" /><path d="M10 9.5l5 2.5-5 2.5z" fill="#ff3b3b" /></svg>
								<h2>Shorts</h2>
							</div>
							<div class="shelf-row">
								{#each shorts as s (s.id)}
									<a class="short" href="/shorts/{s.id}">
										<div class="thumb short-thumb" style="background: {gradientFor(s.id)}"></div>
										<div class="short-title">{s.title}</div>
										<div class="short-views">{s.views} views</div>
									</a>
								{/each}
							</div>
						</section>
					{/if}

					<div class="grid">
						{#each videos.slice(8) as v (v.id)}
							{@render card(v)}
						{/each}
					</div>
				{/if}
			</main>
		</div>
	{/if}
</div>

{#snippet searchForm(variant: 'bar' | 'hero')}
	<form class="search {variant}" role="search" onsubmit={onSearch}>
		<input type="text" placeholder="Search" aria-label="Search" bind:value={query} />
		<button class="search-btn" type="submit" aria-label="Search">
			<svg viewBox="0 0 24 24" width="22" height="22" aria-hidden="true"><circle cx="11" cy="11" r="7" fill="none" stroke="currentColor" stroke-width="2" /><path d="M20 20l-4.2-4.2" stroke="currentColor" stroke-width="2" stroke-linecap="round" /></svg>
		</button>
	</form>
{/snippet}

{#snippet card(v: Video)}
	<a class="card" href="/watch?v={v.id}">
		<div class="thumb" style="background: {gradientFor(v.id)}">
			<span class="dur">{v.duration}</span>
		</div>
		<div class="meta">
			<div class="avatar sm" style="background: {gradientFor(v.channel)}">{v.channel.charAt(0)}</div>
			<div class="text">
				<h3 class="title">{v.title}</h3>
				<div class="sub">{v.channel}</div>
				<div class="sub">{v.views} • {v.age}</div>
			</div>
		</div>
	</a>
{/snippet}

{#snippet compact(v: Video)}
	<a class="up" href="/watch?v={v.id}">
		<div class="thumb" style="background: {gradientFor(v.id)}">
			<span class="dur">{v.duration}</span>
		</div>
		<div class="up-text">
			<h3 class="up-title">{v.title}</h3>
			<div class="sub">{v.channel}</div>
			<div class="sub">{v.views} • {v.age}</div>
		</div>
	</a>
{/snippet}

<style>
	.yt {
		--yt-font: Roboto, Arial, 'Inter Variable', ui-sans-serif, sans-serif;
		min-height: 100%;
		background: #0f0f0f;
		color: #f1f1f1;
		font-family: Roboto, Arial, 'Inter Variable', ui-sans-serif, sans-serif;
		font-size: 14px;
		line-height: 20px;
	}

	button { font: inherit; color: inherit; }

	h1,
	h2,
	h3,
	p { margin: 0; }

	/* Header */
	.top {
		position: sticky;
		top: 0;
		z-index: 10;
		height: 56px;
		padding: 0 16px;
		display: grid;
		grid-template-columns: 1fr minmax(0, 640px) 1fr;
		align-items: center;
		gap: 16px;
		background: #0f0f0f;
	}
	.top-left { display: flex; align-items: center; gap: 8px; min-width: 0; }
	.top-right { display: flex; justify-content: flex-end; }
	.icon-btn {
		width: 40px;
		height: 40px;
		border: 0;
		border-radius: 50%;
		background: transparent;
		display: grid;
		place-items: center;
		cursor: pointer;
	}
	.icon-btn:hover { background: #272727; }
	.brand { display: flex; align-items: center; gap: 4px; color: #f1f1f1; text-decoration: none; }
	.badge { width: 30px; height: 21px; border-radius: 7px; background: #ff0000; display: grid; place-items: center; }
	.wordmark { font: 600 20px/1 var(--yt-font); letter-spacing: -0.5px; }

	.search { display: flex; width: 100%; max-width: 640px; height: 40px; }
	.search input {
		flex: 1;
		min-width: 0;
		height: 100%;
		padding: 0 16px;
		border: 1px solid #303030;
		border-right: 0;
		border-radius: 40px 0 0 40px;
		background: #121212;
		color: #f1f1f1;
		font: 400 16px/1 var(--yt-font);
		outline: none;
	}
	.search input::placeholder { color: #777; }
	.search input:focus { border-color: #1c62b9; }
	.search-btn {
		width: 64px;
		height: 100%;
		flex: none;
		display: grid;
		place-items: center;
		border: 1px solid #303030;
		border-radius: 0 40px 40px 0;
		background: #222;
		cursor: pointer;
	}
	.search-btn:hover { background: #2c2c2c; }
	.search.hero { height: 52px; max-width: 640px; }
	.search.hero input { padding: 0 20px; font-size: 18px; }
	.search.hero .search-btn { width: 84px; }

	.avatar {
		width: 32px;
		height: 32px;
		flex: none;
		border-radius: 50%;
		display: grid;
		place-items: center;
		color: #fff;
		font: 600 14px/1 var(--yt-font);
	}
	.avatar.sm { width: 28px; height: 28px; font-size: 12px; }
	.avatar.lg { width: 40px; height: 40px; }

	/* Shared thumbnail */
	.thumb { position: relative; width: 100%; aspect-ratio: 16 / 9; border-radius: 12px; overflow: hidden; transition: transform 0.2s ease, filter 0.2s ease; }
	.dur {
		position: absolute;
		right: 8px;
		bottom: 8px;
		padding: 2px 4px;
		border-radius: 4px;
		background: rgb(0 0 0 / 0.85);
		color: #fff;
		font: 500 12px/1.2 var(--yt-font);
	}
	.sub { color: #aaa; font-size: 14px; line-height: 20px; }

	/* Home */
	.home { display: flex; align-items: flex-start; }
	.side {
		flex: 0 0 220px;
		position: sticky;
		top: 56px;
		padding: 12px;
		display: flex;
		flex-direction: column;
		gap: 2px;
	}
	.nav {
		height: 40px;
		padding: 0 12px;
		display: flex;
		align-items: center;
		gap: 24px;
		border-radius: 10px;
		color: #f1f1f1;
		text-decoration: none;
		font-weight: 500;
	}
	.nav:hover { background: #272727; }
	.nav.on { background: #272727; font-weight: 600; }
	.main { flex: 1 1 auto; min-width: 0; padding-top: 4px; }
	.chips { display: flex; gap: 12px; padding: 12px 24px 16px; overflow-x: auto; scrollbar-width: none; }
	.chips::-webkit-scrollbar { display: none; }
	.chip {
		flex: none;
		height: 32px;
		padding: 0 12px;
		border: 0;
		border-radius: 8px;
		background: #272727;
		color: #f1f1f1;
		font-weight: 500;
		white-space: nowrap;
		cursor: pointer;
	}
	.chip:hover { background: #3f3f3f; }
	.chip.on { background: #f1f1f1; color: #0f0f0f; }
	.chip.on:hover { background: #d9d9d9; }
	.grid { display: grid; grid-template-columns: repeat(auto-fill, minmax(280px, 1fr)); gap: 32px 16px; padding: 8px 24px 24px; }
	.card { min-width: 0; display: flex; flex-direction: column; gap: 12px; color: inherit; text-decoration: none; }
	.card:hover .thumb,
	.up:hover .thumb,
	.result:hover .thumb,
	.short:hover .short-thumb { transform: scale(1.03); filter: brightness(1.12); }
	.meta { display: flex; gap: 12px; }
	.text { min-width: 0; display: flex; flex-direction: column; gap: 2px; }
	.title {
		display: -webkit-box;
		-webkit-line-clamp: 2;		line-clamp: 2;
		-webkit-box-orient: vertical;
		line-clamp: 2;
		overflow: hidden;
		font-size: 16px;
		line-height: 22px;
		font-weight: 500;
	}

	.shelf { padding: 8px 0 24px; }
	.shelf-head { display: flex; align-items: center; gap: 10px; padding: 8px 24px 12px; }
	.shelf-head h2 { font-size: 18px; font-weight: 500; }
	.shelf-row { display: flex; gap: 12px; padding: 0 24px 8px; overflow-x: auto; scrollbar-width: none; }
	.shelf-row::-webkit-scrollbar { display: none; }
	.short { flex: none; width: 188px; color: inherit; text-decoration: none; }
	.short-thumb { width: 188px; aspect-ratio: 9 / 16; }
	.short-title {
		margin-top: 10px;
		display: -webkit-box;
		-webkit-line-clamp: 2;		line-clamp: 2;
		-webkit-box-orient: vertical;
		overflow: hidden;
		font-size: 15px;
		line-height: 20px;
		font-weight: 500;
	}
	.short-views { color: #aaa; font-size: 13px; }
	.notice-row { padding: 8px 24px 24px; }
	.notice-wrap { width: min(420px, 100%); }
	.calm {
		min-height: 70vh;
		padding: 48px 24px;
		display: flex;
		flex-direction: column;
		align-items: center;
		justify-content: center;
		gap: 24px;
	}
	.calm-text { color: #aaa; font-size: 22px; line-height: 1.3; }

	/* Watch */
	.watch { display: flex; align-items: flex-start; gap: 24px; padding: 24px 24px 48px; }
	.watch-main { flex: 1 1 0; min-width: 0; }
	.player { position: relative; width: 100%; aspect-ratio: 16 / 9; border-radius: 12px; overflow: hidden; }
	.big-play {
		position: absolute;
		left: 50%;
		top: 50%;
		transform: translate(-50%, -50%);
		width: 80px;
		height: 80px;
		border: 0;
		border-radius: 50%;
		background: rgb(0 0 0 / 0.55);
		display: grid;
		place-items: center;
		cursor: pointer;
	}
	.big-play:hover { background: rgb(0 0 0 / 0.72); }
	.controls { position: absolute; left: 0; right: 0; bottom: 0; padding: 28px 12px 8px; background: linear-gradient(transparent, rgb(0 0 0 / 0.55)); }
	.time { display: block; margin-bottom: 8px; color: #fff; font-size: 13px; font-weight: 500; text-shadow: 0 1px 2px rgb(0 0 0 / 0.6); }
	.scrub { height: 4px; border-radius: 2px; background: rgb(255 255 255 / 0.35); overflow: hidden; }
	.played { height: 100%; background: #ff0000; }
	.w-title { margin-top: 16px; font-size: 20px; line-height: 28px; font-weight: 600; }
	.w-row { margin-top: 12px; display: flex; flex-wrap: wrap; align-items: center; justify-content: space-between; gap: 16px; }
	.chan { min-width: 0; display: flex; align-items: center; gap: 12px; }
	.chan-name { font-size: 16px; line-height: 20px; font-weight: 600; }
	.chan-subs { color: #aaa; font-size: 13px; }
	.subscribe {
		height: 36px;
		margin-left: 12px;
		padding: 0 16px;
		border: 0;
		border-radius: 18px;
		background: #cc0000;
		color: #fff;
		font-weight: 500;
		cursor: pointer;
	}
	.subscribe:hover { background: #e00000; }
	.actions { display: flex; gap: 8px; }
	.pill {
		height: 36px;
		padding: 0 14px;
		display: inline-flex;
		align-items: center;
		gap: 8px;
		border: 0;
		border-radius: 18px;
		background: #272727;
		font-weight: 500;
		cursor: pointer;
	}
	.pill:hover { background: #3a3a3a; }
	.desc { margin-top: 12px; padding: 12px 16px; border-radius: 12px; background: #272727; }
	.desc-stats { margin-bottom: 6px; font-weight: 600; }
	.block { margin-top: 24px; }
	.comments { margin-top: 24px; }
	.c-head { display: flex; align-items: center; gap: 32px; }
	.c-head h2 { font-size: 20px; line-height: 28px; font-weight: 500; }
	.sort {
		display: inline-flex;
		align-items: center;
		gap: 6px;
		padding: 6px 8px;
		border: 0;
		border-radius: 8px;
		background: none;
		font-weight: 500;
		cursor: pointer;
	}
	.sort:hover { background: #272727; }
	.c-add { margin: 20px 0; display: flex; align-items: center; gap: 12px; }
	.c-placeholder { flex: 1; padding-bottom: 6px; border-bottom: 1px solid #303030; color: #aaa; }
	.comment { margin-bottom: 20px; display: flex; gap: 16px; }
	.c-body { min-width: 0; }
	.c-meta { display: flex; align-items: baseline; gap: 6px; font-size: 13px; }
	.c-handle { font-weight: 600; }
	.c-age { color: #aaa; }
	.comment p { margin: 4px 0 6px; font-size: 14px; line-height: 20px; }
	.c-like { display: inline-flex; align-items: center; gap: 6px; color: #aaa; font-size: 12px; }

	.up-next { flex: 0 0 400px; width: 400px; display: flex; flex-direction: column; gap: 12px; }
	.up-head { font-size: 16px; font-weight: 500; }
	.up { display: flex; gap: 8px; color: inherit; text-decoration: none; }
	.up .thumb { width: 168px; height: 94px; flex: none; aspect-ratio: auto; }
	.up .dur { right: 4px; bottom: 4px; font-size: 11px; }
	.up-text { min-width: 0; display: flex; flex-direction: column; gap: 2px; }
	.up-title {
		display: -webkit-box;
		-webkit-line-clamp: 2;		line-clamp: 2;
		-webkit-box-orient: vertical;
		overflow: hidden;
		font-size: 14px;
		line-height: 18px;
		font-weight: 500;
	}
	.up-text .sub { font-size: 12px; line-height: 16px; }

	/* Results */
	.results { max-width: 960px; margin: 0 auto; padding: 24px; display: flex; flex-direction: column; gap: 20px; }
	.r-head { font-size: 20px; line-height: 28px; font-weight: 500; }
	.result { display: flex; gap: 16px; color: inherit; text-decoration: none; }
	.r-thumb { width: 360px; max-width: 45%; flex: none; }
	.r-text { min-width: 0; display: flex; flex-direction: column; gap: 6px; }
	.r-title {
		display: -webkit-box;
		-webkit-line-clamp: 2;		line-clamp: 2;
		-webkit-box-orient: vertical;
		overflow: hidden;
		font-size: 18px;
		line-height: 24px;
		font-weight: 500;
	}
	.r-meta,
	.r-chan,
	.r-desc { color: #aaa; font-size: 12px; line-height: 18px; }
	.r-chan { margin-top: 4px; display: flex; align-items: center; gap: 8px; }
	.r-desc { display: -webkit-box; -webkit-line-clamp: 2; line-clamp: 2; -webkit-box-orient: vertical; overflow: hidden; }

	/* Shorts */
	.shorts-page { padding: 16px; }
	.centered { min-height: 70vh; display: flex; flex-direction: column; align-items: center; justify-content: center; gap: 16px; }
	.back { color: #3ea6ff; font-weight: 500; text-decoration: none; }
	.back:hover { text-decoration: underline; }
	.short-stage { display: flex; align-items: center; justify-content: center; gap: 16px; }
	.short-card { position: relative; height: 80vh; max-width: 100%; aspect-ratio: 9 / 16; border-radius: 12px; overflow: hidden; }
	.hint { position: absolute; left: 50%; top: 45%; transform: translate(-50%, -50%); opacity: 0.6; }
	.short-overlay { position: absolute; left: 0; right: 0; bottom: 0; padding: 64px 16px 20px; background: linear-gradient(transparent, rgb(0 0 0 / 0.75)); }
	.s-title { font-size: 16px; line-height: 22px; font-weight: 600; }
	.s-views { margin-top: 4px; color: rgb(255 255 255 / 0.8); font-size: 13px; }
	.short-actions { display: flex; flex-direction: column; align-items: center; gap: 18px; }
	.s-act {
		display: flex;
		flex-direction: column;
		align-items: center;
		gap: 6px;
		border: 0;
		background: none;
		cursor: pointer;
	}
	.s-circle { width: 48px; height: 48px; border-radius: 50%; background: #272727; display: grid; place-items: center; }
	.s-act:hover .s-circle { background: #3a3a3a; }
	.s-count { font-size: 12px; font-weight: 500; }

</style>
