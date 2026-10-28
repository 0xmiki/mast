<script lang="ts">
	import { Search } from '@lucide/svelte';
	import Logo from '../Logo.svelte';
	import { hostOf, isInternal, parse } from '#lib/mast/url.ts';
	import { gradientFor } from '#lib/sites/types.ts';

	let { href, size = 16 }: { href: string; size?: number } = $props();

	const BRANDS: Record<string, { bg: string; fg: string; letter: string; serif?: boolean }> = {
		'youtube.com': { bg: '#ff0033', fg: '#fff', letter: '▶' },
		'x.com': { bg: '#000', fg: '#fff', letter: 'X' },
		'reddit.com': { bg: '#ff4500', fg: '#fff', letter: 'r' },
		'news.ycombinator.com': { bg: '#ff6600', fg: '#fff', letter: 'Y' },
		'github.com': { bg: '#f0f3f6', fg: '#0d1117', letter: 'G' },
		'en.wikipedia.org': { bg: '#fff', fg: '#202122', letter: 'W', serif: true }
	};

	const internal = $derived(isInternal(href));
	const search = $derived(internal && parse(href).host === 'search');
	const host = $derived(hostOf(href));
	const brand = $derived(BRANDS[host]);
</script>

{#if search}
	<span class="fav plain" style:width="{size}px" style:height="{size}px"><Search size={size - 3} /></span>
{:else if internal}
	<Logo {size} />
{:else}
	<span
		class="fav"
		class:serif={brand?.serif}
		style:width="{size}px"
		style:height="{size}px"
		style:font-size="{size * 0.62}px"
		style:background={brand?.bg ?? gradientFor(host)}
		style:color={brand?.fg ?? '#fff'}>{brand?.letter ?? host.charAt(0).toUpperCase()}</span
	>
{/if}

<style>
	.fav {
		display: inline-grid;
		place-items: center;
		flex: none;
		border-radius: 4px;
		font-weight: 800;
		line-height: 1;
	}
	.fav.serif {
		font-family: Georgia, serif;
		font-weight: 400;
	}
	.fav.plain {
		background: none;
		color: var(--muted);
	}
</style>
