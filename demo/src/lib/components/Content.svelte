<script lang="ts">
	import type { Component } from 'svelte';
	import { browser } from '#lib/mast/browser.svelte.ts';
	import { focus } from '#lib/mast/focus.svelte.ts';
	import { hostOf, isInternal, parse } from '#lib/mast/url.ts';
	import { matches } from '#lib/mast/url.ts';
	import type { SiteProps } from '#lib/sites/types.ts';
	import YouTube from '#lib/sites/YouTube.svelte';
	import X from '#lib/sites/X.svelte';
	import Reddit from '#lib/sites/Reddit.svelte';
	import HackerNews from '#lib/sites/HackerNews.svelte';
	import GitHub from '#lib/sites/GitHub.svelte';
	import Wikipedia from '#lib/sites/Wikipedia.svelte';
	import Search from '#lib/sites/Search.svelte';
	import Generic from '#lib/sites/Generic.svelte';
	import Blocked from './pages/Blocked.svelte';
	import NewTab from './pages/NewTab.svelte';
	import BlocklistPage from './pages/BlocklistPage.svelte';
	import Settings from './pages/Settings.svelte';
	import HistoryPage from './pages/HistoryPage.svelte';

	const SITES: [string, Component<SiteProps>][] = [
		['youtube.com', YouTube],
		['x.com', X],
		['twitter.com', X],
		['reddit.com', Reddit],
		['news.ycombinator.com', HackerNews],
		['github.com', GitHub],
		['wikipedia.org', Wikipedia]
	];

	const href = $derived(browser.url);
	const url = $derived(parse(href));
	const blocked = $derived(focus.blocks(href));
	const page = $derived(isInternal(href) ? url.host : '');
	const Site = $derived(SITES.find(([d]) => matches(hostOf(href), d))?.[1] ?? Generic);

	/** Links inside pages are plain anchors: resolve them against the tab URL and navigate. */
	function onclick(e: MouseEvent) {
		const a = (e.target as Element).closest('a');
		const raw = a?.getAttribute('href');
		if (!a || !raw || raw.startsWith('#') || e.defaultPrevented) return;
		e.preventDefault();
		const target = new URL(raw, href).href;
		if (e.ctrlKey || e.metaKey || e.button === 1) browser.newTab(target, false);
		else browser.navigate(target);
	}

	function navigate(to: string) {
		browser.navigate(new URL(to, href).href);
	}
</script>

<!-- svelte-ignore a11y_click_events_have_key_events, a11y_no_static_element_interactions -->
<div class="content" {onclick} onauxclick={onclick}>
	{#key `${browser.activeId}|${href}|${browser.reloads}`}
		{#if blocked}
			<Blocked {href} />
		{:else if page === 'newtab'}
			<NewTab />
		{:else if page === 'blocklist'}
			<BlocklistPage />
		{:else if page === 'settings'}
			<Settings />
		{:else if page === 'history'}
			<HistoryPage />
		{:else if page === 'search'}
			<Search {url} clean={[]} {navigate} />
		{:else if page}
			<NewTab />
		{:else}
			<Site {url} clean={[]} {navigate} />
		{/if}
	{/key}
</div>

<style>
	.content {
		position: relative;
		flex: 1;
		min-width: 0;
		overflow: auto;
		background: var(--bg);
	}
</style>
