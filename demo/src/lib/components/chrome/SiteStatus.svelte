<script lang="ts">
	import { Lock } from '@lucide/svelte';
	import { blocks } from '#lib/mast/blocks.svelte.ts';
	import { clock } from '#lib/mast/clock.svelte.ts';
	import { untilText } from '#lib/mast/engine.ts';
	import { hostOf, isInternal } from '#lib/mast/url.ts';

	let { href, onclose }: { href: string; onclose: () => void } = $props();

	const host = $derived(hostOf(href));
	const block = $derived(blocks.forHref(href));
</script>

<svelte:window onkeydown={(e) => e.key === 'Escape' && onclose()} />
<button class="scrim" aria-label="Close" onclick={onclose}></button>
<div class="pop">
	{#if isInternal(href)}
		<strong>Mast page</strong>
		<span>Part of the browser. Nothing here can lift a block.</span>
	{:else}
		<strong>{host}</strong>
		{#if block}
			<span class="on"><Lock size={13} /> Blocked {untilText(block.until, clock.now)}</span>
		{:else}
			<span>Not blocked. Use Block in the toolbar to block it.</span>
		{/if}
	{/if}
</div>

<style>
	.scrim {
		position: fixed;
		inset: 0;
		z-index: 40;
		border: 0;
		background: none;
		cursor: default;
	}
	.pop {
		position: absolute;
		top: calc(100% + 8px);
		left: 0;
		z-index: 41;
		display: grid;
		gap: 6px;
		justify-items: start;
		width: 300px;
		padding: 16px;
		border-radius: var(--r-2);
		background: var(--chrome);
		border: 1px solid var(--line-2);
		box-shadow: 0 18px 50px rgb(0 0 0 / 0.55);
	}
	span {
		display: inline-flex;
		align-items: center;
		gap: 6px;
		color: var(--muted);
		font-size: 12.5px;
	}
	span.on {
		color: var(--ember-2);
	}
</style>
