<script lang="ts">
	import { Plus, X, Minus, Square } from '@lucide/svelte';
	import { browser } from '#lib/mast/browser.svelte.ts';
	import { blocks } from '#lib/mast/blocks.svelte.ts';
	import { titleFor } from '#lib/mast/url.ts';
	import Favicon from './Favicon.svelte';
</script>

<div class="strip">
	<div class="tabs" role="tablist">
		{#each browser.tabs as tab (tab.id)}
			{@const href = browser.urlOf(tab)}
			{@const active = tab.id === browser.activeId}
			{@const blocked = blocks.blocks(href)}
			<div
				class="tab"
				class:active
				class:blocked
				role="tab"
				tabindex="0"
				aria-selected={active}
				onclick={() => browser.activate(tab.id)}
				onauxclick={(e) => e.button === 1 && browser.close(tab.id)}
				onkeydown={(e) => e.key === 'Enter' && browser.activate(tab.id)}
			>
				{#if active}
					<!-- The sail: a straight luff on the left, a curved leech on the right. -->
					<svg class="leech" viewBox="0 0 34 34" preserveAspectRatio="none" aria-hidden="true">
						<path d="M0 0C14 0 18 34 34 34H0Z" />
					</svg>
				{/if}
				<span class="fav"><Favicon {href} /></span>
				<span class="title">{titleFor(href)}</span>
				<button
					class="close"
					aria-label="Close tab"
					onclick={(e) => {
						e.stopPropagation();
						browser.close(tab.id);
					}}><X size={13} strokeWidth={2} /></button
				>
			</div>
		{/each}
		<button class="new" aria-label="New tab" onclick={() => browser.newTab()}><Plus size={15} strokeWidth={1.75} /></button>
	</div>
	<div class="window" aria-hidden="true">
		<span><Minus size={14} strokeWidth={1.5} /></span>
		<span><Square size={11} strokeWidth={1.5} /></span>
		<span><X size={14} strokeWidth={1.5} /></span>
	</div>
</div>

<style>
	.strip {
		display: flex;
		align-items: flex-end;
		height: 40px;
		padding: 0 0 0 10px;
		background: var(--bg);
		user-select: none;
	}
	.tabs {
		display: flex;
		align-items: flex-end;
		flex: 1;
		min-width: 0;
		height: 100%;
	}
	.tab {
		position: relative;
		display: flex;
		align-items: center;
		gap: 9px;
		flex: 0 1 220px;
		min-width: 48px;
		height: 32px;
		padding: 0 6px 0 14px;
		color: var(--faint);
		font-size: 12.5px;
		cursor: default;
	}
	/* Inactive tabs sit on a ruler: one tick at each edge, rising from the baseline. */
	.tab::before {
		content: '';
		position: absolute;
		left: 0;
		bottom: 0;
		width: 1px;
		height: 8px;
		background: var(--line-2);
	}
	.tab:first-child::before,
	.tab.active::before,
	.tab.active + .tab::before {
		display: none;
	}
	.tab:not(.active):hover {
		color: var(--muted);
		background: linear-gradient(var(--chrome) 0 0) bottom / 100% 2px no-repeat;
	}
	.tab.active {
		z-index: 1;
		margin-right: 34px;
		border-radius: 2px 0 0 0;
		background: var(--chrome);
		color: var(--text);
	}
	.leech {
		position: absolute;
		top: 0;
		left: 100%;
		width: 34px;
		height: 100%;
		fill: var(--chrome);
	}
	.fav {
		display: grid;
		flex: none;
	}
	.title {
		flex: 1;
		min-width: 0;
		overflow: hidden;
		white-space: nowrap;
		text-overflow: clip;
		mask-image: linear-gradient(90deg, #000 82%, transparent);
	}
	/* Blocked: struck through with the same ember line as the fuse. */
	.blocked .fav {
		filter: grayscale(1);
		opacity: 0.35;
	}
	.blocked .title {
		color: var(--faint);
		text-decoration: line-through 1.5px var(--ember);
	}
	.close {
		display: grid;
		place-items: center;
		width: 20px;
		height: 20px;
		border: 0;
		border-radius: 2px;
		background: none;
		color: var(--faint);
		cursor: pointer;
		flex: none;
		opacity: 0;
	}
	.tab:hover .close,
	.tab.active .close {
		opacity: 1;
	}
	.close:hover {
		background: var(--raised-2);
		color: var(--text);
	}
	.new {
		display: grid;
		place-items: center;
		width: 28px;
		height: 28px;
		margin: 0 0 2px 4px;
		border: 0;
		border-radius: var(--r);
		background: none;
		color: var(--faint);
		cursor: pointer;
		flex: none;
	}
	.new:hover {
		background: var(--chrome-2);
		color: var(--text);
	}
	.window {
		display: flex;
		align-self: stretch;
		color: var(--faint);
	}
	.window span {
		display: grid;
		place-items: center;
		width: 44px;
	}
</style>
