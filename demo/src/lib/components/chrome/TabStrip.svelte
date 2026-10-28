<script lang="ts">
	import { Plus, X, Lock } from '@lucide/svelte';
	import { browser } from '#lib/mast/browser.svelte.ts';
	import { focus } from '#lib/mast/focus.svelte.ts';
	import { titleFor } from '#lib/mast/url.ts';
	import Favicon from './Favicon.svelte';

</script>

<div class="strip">
	<div class="tabs" role="tablist">
		{#each browser.tabs as tab (tab.id)}
			{@const href = browser.urlOf(tab)}
			{@const active = tab.id === browser.activeId}
			<div
				class="tab"
				class:active
				role="tab"
				tabindex="0"
				aria-selected={active}
				onclick={() => browser.activate(tab.id)}
				onauxclick={(e) => e.button === 1 && browser.close(tab.id)}
				onkeydown={(e) => e.key === 'Enter' && browser.activate(tab.id)}
			>
				{#if focus.blocks(href)}
					<span class="sealed"><Lock size={11} strokeWidth={2.6} /></span>
				{:else}
					<Favicon {href} />
				{/if}
				<span class="title">{titleFor(href)}</span>
				<button
					class="close"
					aria-label="Close tab"
					onclick={(e) => {
						e.stopPropagation();
						browser.close(tab.id);
					}}><X size={13} strokeWidth={2.4} /></button
				>
			</div>
		{/each}
		<button class="new" aria-label="New tab" onclick={() => browser.newTab()}><Plus size={16} /></button>
	</div>
	<div class="window" aria-hidden="true">
		<span></span><span></span><span></span>
	</div>
</div>

<style>
	.strip {
		display: flex;
		align-items: flex-end;
		height: 42px;
		padding: 0 8px 0 8px;
		background: var(--bg);
		user-select: none;
	}
	.tabs {
		display: flex;
		align-items: flex-end;
		gap: 2px;
		flex: 1;
		min-width: 0;
		height: 100%;
	}
	.tab {
		position: relative;
		display: flex;
		align-items: center;
		gap: 8px;
		flex: 0 1 230px;
		min-width: 52px;
		height: 34px;
		padding: 0 6px 0 12px;
		border-radius: 10px 10px 0 0;
		color: var(--muted);
		font-size: 12.5px;
		cursor: default;
		transition: background 0.12s;
	}
	.tab:not(.active):hover {
		background: rgb(255 255 255 / 0.04);
	}
	.tab:not(.active):not(:hover) + .tab:not(.active)::before {
		content: '';
		position: absolute;
		left: -1px;
		top: 9px;
		bottom: 9px;
		width: 1px;
		background: var(--line-2);
	}
	.tab.active {
		background: var(--chrome);
		color: var(--text);
	}
	.tab.active::after,
	.tab.active::before {
		content: '';
		position: absolute;
		bottom: 0;
		width: 10px;
		height: 10px;
		background: radial-gradient(circle at var(--c) 0, transparent 9.5px, var(--chrome) 10px);
	}
	.tab.active::before {
		left: -10px;
		--c: 0;
	}
	.tab.active::after {
		right: -10px;
		--c: 100%;
	}
	.title {
		flex: 1;
		min-width: 0;
		overflow: hidden;
		white-space: nowrap;
		text-overflow: ellipsis;
		mask-image: linear-gradient(90deg, #000 85%, transparent);
	}
	.sealed {
		display: grid;
		place-items: center;
		width: 16px;
		height: 16px;
		border-radius: 4px;
		background: var(--ember-soft);
		color: var(--ember);
		flex: none;
	}
	.close {
		display: grid;
		place-items: center;
		width: 20px;
		height: 20px;
		border: 0;
		border-radius: 6px;
		background: none;
		color: var(--faint);
		cursor: pointer;
		flex: none;
	}
	.close:hover {
		background: var(--raised-2);
		color: var(--text);
	}
	.new {
		display: grid;
		place-items: center;
		width: 30px;
		height: 30px;
		margin: 0 0 2px 6px;
		border: 0;
		border-radius: 8px;
		background: none;
		color: var(--muted);
		cursor: pointer;
		flex: none;
	}
	.new:hover {
		background: var(--raised);
		color: var(--text);
	}
	.window {
		display: flex;
		gap: 8px;
		align-self: center;
		padding: 0 6px 0 16px;
	}
	.window span {
		width: 12px;
		height: 12px;
		border-radius: 50%;
		background: var(--raised-2);
	}
</style>
