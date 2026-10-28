<script lang="ts">
	import { Plus } from '@lucide/svelte';
	import { browser } from '#lib/mast/browser.svelte.ts';
	import { pacts } from '#lib/mast/pacts.svelte.ts';
	import { clock } from '#lib/mast/clock.svelte.ts';
	import { status, DAY } from '#lib/mast/engine.ts';
	import PactRow from '../PactRow.svelte';
	import Favicon from '../chrome/Favicon.svelte';

	const open = $derived(pacts.open);
	const kept = $derived(pacts.closed.filter((p) => status(p, clock.now) === 'kept'));
	const broken = $derived(pacts.closed.filter((p) => status(p, clock.now) === 'released'));

	/** Attempts per site over the last 7 days. */
	const bySite = $derived.by(() => {
		const since = clock.now - 7 * DAY;
		const counts = new Map<string, number>();
		for (const a of pacts.attempts) if (a.at >= since) counts.set(a.host, (counts.get(a.host) ?? 0) + 1);
		return [...counts].sort((a, b) => b[1] - a[1]).slice(0, 6);
	});
	const max = $derived(Math.max(1, ...bySite.map(([, n]) => n)));
	const keptRate = $derived(kept.length + broken.length ? Math.round((kept.length / (kept.length + broken.length)) * 100) : null);
</script>

<div class="page">
	<div class="wrap">
		<div class="head">
			<div>
				<p class="eyebrow">Ledger</p>
				<h1>Your pacts</h1>
			</div>
			<button class="btn primary" onclick={() => browser.newPact()}><Plus size={16} /> New pact</button>
		</div>

		<div class="stats">
			<div class="card"><b class="tabular">{pacts.live.length}</b><span>In force now</span></div>
			<div class="card"><b class="tabular">{pacts.attemptsToday()}</b><span>Walls hit today</span></div>
			<div class="card"><b class="tabular">{kept.length}</b><span>Kept</span></div>
			<div class="card"><b class="tabular">{keptRate === null ? '—' : keptRate + '%'}</b><span>Word kept</span></div>
		</div>

		<div class="cols">
			<section>
				<h2 class="eyebrow">Open</h2>
				{#if open.length}
					<div class="list">{#each open as p (p.id)}<PactRow pact={p} />{/each}</div>
				{:else}
					<div class="empty">
						Nothing sealed. <button class="link" onclick={() => browser.newPact()}>Make your first pact</button>
					</div>
				{/if}

				{#if kept.length || broken.length}
					<h2 class="eyebrow">History</h2>
					<div class="list">{#each [...kept, ...broken] as p (p.id)}<PactRow pact={p} />{/each}</div>
				{/if}
			</section>

			<aside class="card">
				<h3>Walls hit · last 7 days</h3>
				{#if bySite.length}
					<ul>
						{#each bySite as [host, n]}
							<li>
								<span class="site"><Favicon href="https://{host}" size={14} />{host}</span>
								<span class="bar"><span style:width="{(n / max) * 100}%"></span></span>
								<b class="tabular">{n}</b>
							</li>
						{/each}
					</ul>
					<p class="note">Every time a pact stops you, it’s counted here. Watching the number fall is the point.</p>
				{:else}
					<p class="note">No attempts yet.</p>
				{/if}
			</aside>
		</div>
	</div>
</div>

<style>
	.page {
		min-height: 100%;
		padding: 36px 24px 64px;
		background: var(--bg);
	}
	.wrap {
		max-width: 980px;
		margin: 0 auto;
	}
	.head {
		display: flex;
		align-items: flex-end;
		justify-content: space-between;
	}
	.head p {
		margin: 0 0 4px;
	}
	h1 {
		margin: 0;
		font-size: 30px;
		letter-spacing: -0.02em;
	}
	.stats {
		display: grid;
		grid-template-columns: repeat(4, 1fr);
		gap: 12px;
		margin-top: 24px;
	}
	.stats .card {
		display: grid;
		padding: 16px 18px;
	}
	.stats b {
		font-size: 28px;
		font-weight: 700;
		letter-spacing: -0.02em;
	}
	.stats span {
		color: var(--muted);
		font-size: 12.5px;
	}
	.cols {
		display: grid;
		grid-template-columns: 1fr 320px;
		gap: 20px;
		margin-top: 12px;
		align-items: start;
	}
	h2 {
		margin: 22px 0 10px;
	}
	.list {
		display: grid;
		gap: 8px;
	}
	.empty {
		padding: 20px;
		border-radius: 14px;
		border: 1px dashed var(--line-2);
		color: var(--muted);
	}
	.link {
		border: 0;
		background: none;
		color: var(--ember-2);
		cursor: pointer;
		font-weight: 600;
	}
	aside {
		margin-top: 22px;
		padding: 18px;
	}
	h3 {
		margin: 0 0 14px;
		font-size: 13.5px;
	}
	ul {
		display: grid;
		gap: 10px;
		margin: 0;
		padding: 0;
		list-style: none;
	}
	li {
		display: grid;
		grid-template-columns: 140px 1fr 24px;
		align-items: center;
		gap: 10px;
		font-size: 12.5px;
	}
	.site {
		display: flex;
		align-items: center;
		gap: 8px;
		overflow: hidden;
		white-space: nowrap;
		text-overflow: ellipsis;
	}
	.bar {
		height: 6px;
		border-radius: 99px;
		background: var(--raised-2);
		overflow: hidden;
	}
	.bar span {
		display: block;
		height: 100%;
		background: var(--ember);
		border-radius: inherit;
	}
	li b {
		text-align: right;
	}
	.note {
		margin: 14px 0 0;
		color: var(--faint);
		font-size: 12px;
	}
</style>
