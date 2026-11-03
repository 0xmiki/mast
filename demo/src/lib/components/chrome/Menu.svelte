<script lang="ts">
	import {
		Plus,
		AppWindow,
		VenetianMask,
		History,
		Settings,
		Puzzle,
		Code,
		Info,
		Lock,
		ScrollText
	} from '@lucide/svelte';
	import { browser } from '#lib/mast/browser.svelte.ts';
	import { blocks } from '#lib/mast/blocks.svelte.ts';
	import { attemptEscape } from '#lib/mast/escapes.ts';

	let { onclose }: { onclose: () => void } = $props();

	function run(fn: () => void) {
		fn();
		onclose();
	}
</script>

<svelte:window onkeydown={(e) => e.key === 'Escape' && onclose()} />
<button class="scrim" aria-label="Close menu" onclick={onclose}></button>
<div class="menu" role="menu">
	<button role="menuitem" onclick={() => run(() => browser.newTab())}><Plus size={16} /> New tab</button>
	<button role="menuitem" onclick={() => run(() => browser.toast('New windows open in the real browser.'))}>
		<AppWindow size={16} /> New window
	</button>
	<button role="menuitem" class:locked={blocks.any} onclick={() => run(() => attemptEscape('incognito'))}>
		<VenetianMask size={16} /> New incognito window
		{#if blocks.any}<Lock size={13} class="lock" />{/if}
	</button>
	<hr />
	<button role="menuitem" onclick={() => run(() => browser.navigate('mast://blocklist'))}>
		<ScrollText size={16} /> Blocklist
	</button>
	<button role="menuitem" onclick={() => run(() => browser.navigate('mast://history'))}>
		<History size={16} /> History
	</button>
	<hr />
	<button role="menuitem" onclick={() => run(() => attemptEscape('extensions'))}>
		<Puzzle size={16} /> Extensions
	</button>
	<button role="menuitem" class:locked={blocks.any} onclick={() => run(() => attemptEscape('devtools'))}>
		<Code size={16} /> Developer tools
		{#if blocks.any}<Lock size={13} class="lock" />{/if}
	</button>
	<button role="menuitem" onclick={() => run(() => browser.navigate('mast://settings'))}>
		<Settings size={16} /> Settings
	</button>
	<hr />
	<button
		role="menuitem"
		onclick={() =>
			run(
				() =>
					(browser.dialog = {
						icon: 'info',
						title: 'Mast 0.1 · demo',
						body: 'One button, no distractions. This is the interactive mock used to design the product before the Chromium fork.'
					})
			)}
	>
		<Info size={16} /> About Mast
	</button>
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
	.menu {
		position: absolute;
		top: calc(100% + 6px);
		right: 0;
		z-index: 41;
		width: 260px;
		padding: 6px;
		border-radius: var(--r-2);
		background: var(--chrome);
		border: 1px solid var(--line-2);
		box-shadow: 0 18px 50px rgb(0 0 0 / 0.55);
	}
	button {
		display: flex;
		align-items: center;
		gap: 12px;
		width: 100%;
		height: 34px;
		padding: 0 12px;
		border: 0;
		border-radius: 2px;
		background: none;
		text-align: left;
		cursor: pointer;
		font-size: 13px;
	}
	button:hover {
		background: var(--raised);
	}
	button :global(svg) {
		color: var(--muted);
	}
	button.locked {
		color: var(--faint);
	}
	button :global(.lock) {
		margin-left: auto;
		color: var(--ember);
	}
	hr {
		border: 0;
		border-top: 1px solid var(--line);
		margin: 6px 4px;
	}
</style>
