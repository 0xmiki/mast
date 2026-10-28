<script lang="ts">
	import { ArrowLeft, ArrowRight, RotateCw, EllipsisVertical, Lock } from '@lucide/svelte';
	import { browser } from '#lib/mast/browser.svelte.ts';
	import { focus } from '#lib/mast/focus.svelte.ts';
	import { clock } from '#lib/mast/clock.svelte.ts';
	import { countdown } from '#lib/mast/engine.ts';
	import Omnibox from './Omnibox.svelte';
	import Menu from './Menu.svelte';
	import Logo from '../Logo.svelte';
	import StartControl from '../StartControl.svelte';

	let menuOpen = $state(false);
	let startOpen = $state(false);
	const tab = $derived(browser.active);
	const active = $derived(focus.active);
</script>

<svelte:window onkeydown={(e) => e.key === 'Escape' && (startOpen = false)} />

<div class="bar" class:running={!!active}>
	<div class="nav">
		<button class="icon" aria-label="Back" disabled={tab.index === 0} onclick={() => browser.back()}>
			<ArrowLeft size={17} />
		</button>
		<button
			class="icon"
			aria-label="Forward"
			disabled={tab.index >= tab.history.length - 1}
			onclick={() => browser.forward()}
		>
			<ArrowRight size={17} />
		</button>
		<button class="icon" aria-label="Reload" onclick={() => browser.reload()}><RotateCw size={15} /></button>
	</div>

	<Omnibox />

	<div class="end">
		<div class="pop-wrap">
			{#if active}
				<button class="session tabular" class:on={startOpen} onclick={() => (startOpen = !startOpen)}>
					<Lock size={13} strokeWidth={2.6} />
					{countdown(active.endsAt - clock.now)}
				</button>
			{:else}
				<button class="start" class:on={startOpen} onclick={() => (startOpen = !startOpen)}>
					<Logo size={16} /> Block
				</button>
			{/if}
			{#if startOpen}
				<button class="scrim" aria-label="Close" onclick={() => (startOpen = false)}></button>
				<div class="pop"><StartControl compact /></div>
			{/if}
		</div>
		<div class="pop-wrap">
			<button class="icon" aria-label="Menu" class:on={menuOpen} onclick={() => (menuOpen = !menuOpen)}>
				<EllipsisVertical size={17} />
			</button>
			{#if menuOpen}<Menu onclose={() => (menuOpen = false)} />{/if}
		</div>
	</div>
</div>

<style>
	.bar {
		position: relative;
		display: flex;
		align-items: center;
		gap: 8px;
		height: 46px;
		padding: 0 8px;
		background: var(--chrome);
		border-bottom: 1px solid var(--line);
	}
	.bar.running::after {
		content: '';
		position: absolute;
		left: 0;
		right: 0;
		bottom: -1px;
		height: 2px;
		background: linear-gradient(90deg, transparent, var(--ember), transparent);
		opacity: 0.7;
	}
	.nav,
	.end {
		display: flex;
		align-items: center;
		gap: 4px;
	}
	button {
		border: 0;
		cursor: pointer;
	}
	.icon {
		display: grid;
		place-items: center;
		width: 32px;
		height: 32px;
		border-radius: 999px;
		background: none;
		color: var(--muted);
	}
	.icon:hover:not(:disabled),
	.icon.on {
		background: var(--raised);
		color: var(--text);
	}
	.icon:disabled {
		opacity: 0.35;
		cursor: default;
	}
	.start,
	.session {
		display: inline-flex;
		align-items: center;
		gap: 7px;
		height: 32px;
		padding: 0 12px 0 9px;
		border-radius: 999px;
		font-size: 13px;
		font-weight: 650;
		white-space: nowrap;
	}
	.start {
		background: var(--raised);
		color: var(--text);
	}
	.start:hover,
	.start.on {
		background: var(--raised-2);
	}
	.session {
		padding: 0 12px;
		background: var(--ember-soft);
		color: var(--ember-2);
		font-family: var(--mono);
	}
	.session:hover,
	.session.on {
		background: rgb(255 106 61 / 0.22);
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
		top: calc(100% + 8px);
		right: 0;
		z-index: 41;
		width: 340px;
		padding: 20px 16px 16px;
		border-radius: 18px;
		background: var(--chrome);
		border: 1px solid var(--line-2);
		box-shadow: 0 20px 60px rgb(0 0 0 / 0.55);
	}
</style>
