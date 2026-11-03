<script lang="ts">
	/**
	 * The horizon: the one line between the browser and the web.
	 * It is a hairline, except on a blocked site: then it carries the block itself,
	 * a fuse burning down for a timed block, a solid ember line for a forever one.
	 */
	import { browser } from '#lib/mast/browser.svelte.ts';
	import { blocks } from '#lib/mast/blocks.svelte.ts';
	import Fuse from './Fuse.svelte';

	const b = $derived(blocks.forHref(browser.url));
</script>

<div class="horizon">
	{#if b}<Fuse block={b} width="100%" thick={2} />{/if}
</div>

<style>
	.horizon {
		position: relative;
		display: flex;
		align-items: center;
		height: 1px;
		background: var(--line);
		flex: none;
		z-index: 2;
	}
</style>
