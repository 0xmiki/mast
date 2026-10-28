<script lang="ts">
	import { ArrowLeft, ArrowRight, RotateCw, EllipsisVertical, Crosshair } from '@lucide/svelte';
	import { browser } from '#lib/mast/browser.svelte.ts';
	import { pacts } from '#lib/mast/pacts.svelte.ts';
	import { clock } from '#lib/mast/clock.svelte.ts';
	import { formatDuration } from '#lib/mast/engine.ts';
	import Omnibox from './Omnibox.svelte';
	import Menu from './Menu.svelte';
	import Logo from '../Logo.svelte';

	let menuOpen = $state(false);
	const tab = $derived(browser.active);
	const focus = $derived(pacts.focus);
</script>

<div class="bar" class:focus={!!focus}>
	<div class="nav">
		<button aria-label="Back" disabled={tab.index === 0} onclick={() => browser.back()}>
			<ArrowLeft size={17} />
		</button>
		<button
			aria-label="Forward"
			disabled={tab.index >= tab.history.length - 1}
			onclick={() => browser.forward()}
		>
			<ArrowRight size={17} />
		</button>
		<button aria-label="Reload" onclick={() => browser.reload()}><RotateCw size={15} /></button>
	</div>

	<Omnibox />

	{#if focus}
		<a
			class="focus-pill tabular"
			href="mast://pacts/{focus.id}"
			onclick={(e) => {
				e.preventDefault();
				browser.navigate(`mast://pacts/${focus.id}`);
			}}
		>
			<Crosshair size={14} /> Focus · {formatDuration(focus.endsAt - clock.now)}
		</a>
	{/if}

	<div class="end">
		<button
			class="pacts"
			class:on={browser.panelOpen}
			aria-label="Pacts panel"
			title="Pacts"
			onclick={() => (browser.panelOpen = !browser.panelOpen)}
		>
			<Logo size={18} />
			{#if pacts.live.length}<span class="count">{pacts.live.length}</span>{/if}
		</button>
		<div class="menu-wrap">
			<button aria-label="Menu" class:on={menuOpen} onclick={() => (menuOpen = !menuOpen)}>
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
	.bar.focus::after {
		content: '';
		position: absolute;
		left: 0;
		right: 0;
		bottom: -1px;
		height: 2px;
		background: linear-gradient(90deg, transparent, var(--focus), transparent);
		opacity: 0.8;
	}
	.nav,
	.end {
		display: flex;
		align-items: center;
		gap: 2px;
	}
	button {
		position: relative;
		display: grid;
		place-items: center;
		width: 32px;
		height: 32px;
		border: 0;
		border-radius: 999px;
		background: none;
		color: var(--muted);
		cursor: pointer;
	}
	button:hover:not(:disabled),
	button.on {
		background: var(--raised);
		color: var(--text);
	}
	button:disabled {
		opacity: 0.35;
		cursor: default;
	}
	.pacts {
		width: 36px;
	}
	.count {
		position: absolute;
		top: 0;
		right: -3px;
		min-width: 14px;
		height: 14px;
		padding: 0 3px;
		border-radius: 999px;
		background: var(--text);
		color: var(--bg);
		font-size: 9.5px;
		font-weight: 800;
		line-height: 14px;
		border: 2px solid var(--chrome);
		box-sizing: content-box;
	}
	.menu-wrap {
		position: relative;
	}
	.focus-pill {
		display: inline-flex;
		align-items: center;
		gap: 6px;
		height: 30px;
		padding: 0 12px;
		border-radius: 999px;
		background: var(--focus-soft);
		color: var(--focus);
		font-size: 12.5px;
		font-weight: 650;
		text-decoration: none;
		white-space: nowrap;
	}
</style>
