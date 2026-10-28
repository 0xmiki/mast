<script lang="ts">
	import { browser } from '#lib/mast/browser.svelte.ts';
	import { pacts } from '#lib/mast/pacts.svelte.ts';
	import { clock } from '#lib/mast/clock.svelte.ts';
	import { formatWhen } from '#lib/mast/engine.ts';
	import { titleFor } from '#lib/mast/url.ts';
	import Favicon from '../chrome/Favicon.svelte';

	const entries = $derived([...browser.history].reverse().slice(0, 100));
</script>

<div class="page">
	<div class="wrap">
		<h1>History</h1>
		<p class="sub">Clearing history works as usual. Pacts and the ledger aren’t part of it.</p>
		<div class="card">
			{#each entries as e}
				<a href={e.href}>
					<span class="when tabular">{formatWhen(e.at, clock.now)}</span>
					<Favicon href={e.href} size={15} />
					<span class="title">{titleFor(e.href)}</span>
					<span class="url">{e.href}</span>
				</a>
			{:else}
				<p class="empty">Nothing yet.</p>
			{/each}
		</div>
		{#if entries.length}
			<button
				class="btn small"
				onclick={() => {
					browser.clearHistory();
					browser.toast(`History cleared. ${pacts.live.length} pacts untouched.`, 'good');
				}}>Clear browsing data</button
			>
		{/if}
	</div>
</div>

<style>
	.page {
		min-height: 100%;
		padding: 36px 24px 64px;
		background: var(--bg);
	}
	.wrap {
		max-width: 820px;
		margin: 0 auto;
	}
	h1 {
		margin: 0;
		font-size: 28px;
	}
	.sub {
		margin: 6px 0 20px;
		color: var(--muted);
	}
	.card {
		padding: 6px;
		margin-bottom: 14px;
	}
	a {
		display: flex;
		align-items: center;
		gap: 12px;
		padding: 8px 12px;
		border-radius: 8px;
		color: var(--text);
		text-decoration: none;
		font-size: 13px;
	}
	a:hover {
		background: var(--raised);
	}
	.when {
		width: 130px;
		color: var(--faint);
		font-size: 12px;
		flex: none;
	}
	.title {
		flex: none;
		max-width: 220px;
		overflow: hidden;
		text-overflow: ellipsis;
		white-space: nowrap;
	}
	.url {
		color: var(--faint);
		overflow: hidden;
		text-overflow: ellipsis;
		white-space: nowrap;
	}
	.empty {
		padding: 12px;
		color: var(--muted);
	}
</style>
