<script lang="ts">
	import { Plus, Lock } from '@lucide/svelte';
	import { browser } from '#lib/mast/browser.svelte.ts';
	import { focus } from '#lib/mast/focus.svelte.ts';
	import { isBlocked } from '#lib/mast/engine.ts';
	import { hostOf, isInternal } from '#lib/mast/url.ts';

	let { href, onclose }: { href: string; onclose: () => void } = $props();

	const host = $derived(hostOf(href));
	const listed = $derived(isBlocked(href, focus.blocklist));
</script>

<svelte:window onkeydown={(e) => e.key === 'Escape' && onclose()} />
<button class="scrim" aria-label="Close" onclick={onclose}></button>
<div class="pop">
	{#if isInternal(href)}
		<strong>Mast page</strong>
		<span>Part of the browser. Nothing here can stop a running session.</span>
	{:else}
		<strong>{host}</strong>
		{#if listed}
			<span class="on"><Lock size={13} /> On your blocklist{focus.active ? ' · blocked right now' : ''}</span>
		{:else}
			<span>Not on your blocklist. Opens during sessions.</span>
			<button
				class="btn small"
				onclick={() => {
					focus.addSite(host);
					browser.toast(`${host} added to your blocklist.`, 'good');
					onclose();
				}}><Plus size={14} /> Add to blocklist</button
			>
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
		border-radius: 14px;
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
	.btn {
		margin-top: 6px;
	}
</style>
