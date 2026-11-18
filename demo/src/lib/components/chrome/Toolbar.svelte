<script lang="ts">
	import { ArrowLeft, ArrowRight, RotateCw, Menu as MenuIcon, Lock } from '@lucide/svelte';
	import { browser } from '#lib/mast/browser.svelte.ts';
	import { blocks } from '#lib/mast/blocks.svelte.ts';
	import { clock } from '#lib/mast/clock.svelte.ts';
	import { untilText } from '#lib/mast/engine.ts';
	import { hostOf } from '#lib/mast/url.ts';
	import Omnibox from './Omnibox.svelte';
	import Menu from './Menu.svelte';
	import BlockAction from '../BlockAction.svelte';
	import Horizon from '../Horizon.svelte';
	import Logo from '../Logo.svelte';

	let menuOpen = $state(false);
	let blockOpen = $state(false);
	const tab = $derived(browser.active);
	const host = $derived(hostOf(browser.url));
	const entry = $derived(host ? blocks.entryFor(host) : '');
	const current = $derived(blocks.forHref(browser.url));

	$effect(() => {
		void browser.url;
		blockOpen = false;
	});
</script>

<svelte:window onkeydown={(e) => e.key === 'Escape' && (blockOpen = false)} />

<div class="bar">
	<div class="nav">
		<button class="icon" aria-label="Back" disabled={tab.index === 0} onclick={() => browser.back()}>
			<ArrowLeft size={16} strokeWidth={1.75} />
		</button>
		<button
			class="icon"
			aria-label="Forward"
			disabled={tab.index >= tab.history.length - 1}
			onclick={() => browser.forward()}
		>
			<ArrowRight size={16} strokeWidth={1.75} />
		</button>
		<button class="icon" aria-label="Reload" onclick={() => browser.reload()}>
			<RotateCw size={14} strokeWidth={1.75} />
		</button>
	</div>

	<div class="mid"><Omnibox /></div>

	<div class="end">
		{#if host}
			<div class="pop-wrap">
				{#if current}
					<button class="readout mono" class:on={blockOpen} onclick={() => (blockOpen = !blockOpen)}>
						<Lock size={12} strokeWidth={2.4} />
						{current.until === null ? 'forever' : untilText(current.until, clock.now).replace('until ', '')}
					</button>
				{:else}
					<button class="block" class:on={blockOpen} onclick={() => (blockOpen = !blockOpen)}>
						<Logo size={15} /> Block
					</button>
				{/if}
				{#if blockOpen}
					<button class="scrim" aria-label="Close" onclick={() => (blockOpen = false)}></button>
					<div class="pop">
						<p class="eyebrow">{current ? 'Blocked' : 'Block this site'}</p>
						<p class="site">{entry}</p>
						{#if current?.until === null}
							<p class="hint">Blocked forever. That can’t change.</p>
						{:else}
							{#if current}<p class="hint">Blocked {untilText(current.until, clock.now)}. You can make it longer, never shorter.</p>{/if}
							<BlockAction compact domains={[entry]} what="" onblocked={() => (blockOpen = false)} />
						{/if}
					</div>
				{/if}
			</div>
		{/if}
		<div class="pop-wrap">
			<button class="icon" aria-label="Menu" class:on={menuOpen} onclick={() => (menuOpen = !menuOpen)}>
				<MenuIcon size={16} strokeWidth={1.75} />
			</button>
			{#if menuOpen}<Menu onclose={() => (menuOpen = false)} />{/if}
		</div>
	</div>
</div>
<Horizon />

<style>
	/* An instrument panel: three cells divided by hairlines, the address centred in the middle one. */
	.bar {
		display: grid;
		grid-template-columns: minmax(max-content, 1fr) minmax(0, 820px) minmax(max-content, 1fr);
		align-items: stretch;
		height: 44px;
		background: var(--chrome);
	}
	.nav,
	.end {
		display: flex;
		align-items: center;
		gap: 2px;
		padding: 0 8px;
	}
	.end {
		justify-content: flex-end;
	}
	.mid {
		display: flex;
		align-items: center;
		padding: 0 10px;
		box-shadow:
			-1px 0 0 var(--line),
			1px 0 0 var(--line);
	}
	button {
		border: 0;
		cursor: pointer;
	}
	.icon {
		display: grid;
		place-items: center;
		width: 30px;
		height: 30px;
		border-radius: var(--r);
		background: none;
		color: var(--muted);
	}
	.icon:hover:not(:disabled),
	.icon.on {
		background: var(--raised);
		color: var(--text);
	}
	.icon:disabled {
		color: var(--faint);
		opacity: 0.45;
		cursor: default;
	}
	.block {
		display: inline-flex;
		align-items: center;
		gap: 8px;
		height: 30px;
		margin-right: 4px;
		padding: 0 12px 0 9px;
		border-radius: var(--r);
		background: none;
		box-shadow: inset 0 0 0 1px var(--line-2);
		color: var(--muted);
		font-size: 12.5px;
		font-weight: 600;
	}
	.block:hover,
	.block.on {
		background: var(--raised);
		color: var(--text);
	}
	.site {
		margin: 6px 0 0;
		font-size: 20px;
		font-weight: 600;
		letter-spacing: -0.02em;
	}
	.hint {
		margin: 6px 0 0;
		font-size: 12.5px;
		color: var(--muted);
	}
	.pop .eyebrow {
		margin: 0;
	}
	.pop :global(.act) {
		margin-top: 18px;
	}
	.readout {
		display: inline-flex;
		align-items: center;
		gap: 8px;
		height: 30px;
		margin-right: 4px;
		padding: 0 11px 0 10px;
		border-radius: var(--r);
		background: none;
		box-shadow: inset 0 0 0 1px rgb(255 143 99 / 0.28);
		color: var(--ember-2);
		font-size: 12.5px;
		font-weight: 600;
		white-space: nowrap;
	}
	.readout:hover,
	.readout.on {
		background: var(--ember-soft);
	}
	.pop-wrap {
		position: relative;
	}
	.scrim {
		position: fixed;
		inset: 0;
		z-index: 40;
		background: none;
		cursor: default;
	}
	.pop {
		position: absolute;
		top: calc(100% + 7px);
		right: 0;
		z-index: 41;
		width: 320px;
		padding: 20px 18px 18px;
		border-radius: var(--r-2);
		background: var(--chrome);
		border: 1px solid var(--line-2);
		box-shadow: 0 24px 60px rgb(0 0 0 / 0.55);
	}
</style>
