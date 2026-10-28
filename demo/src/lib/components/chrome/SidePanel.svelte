<script lang="ts">
	import { X, Plus, Ban, Hourglass, Crosshair, Wind, Sparkles } from '@lucide/svelte';
	import { browser } from '#lib/mast/browser.svelte.ts';
	import { pacts } from '#lib/mast/pacts.svelte.ts';
	import { clock } from '#lib/mast/clock.svelte.ts';
	import { status } from '#lib/mast/engine.ts';
	import type { PactKind } from '#lib/mast/types.ts';
	import PactRow from '../PactRow.svelte';
	import Logo from '../Logo.svelte';

	const open = $derived(pacts.open);
	const live = $derived(open.filter((p) => status(p, clock.now) !== 'upcoming'));
	const upcoming = $derived(open.filter((p) => status(p, clock.now) === 'upcoming'));

	const QUICK: { kind: PactKind; label: string; icon: typeof Ban }[] = [
		{ kind: 'focus', label: 'Focus session', icon: Crosshair },
		{ kind: 'block', label: 'Block sites', icon: Ban },
		{ kind: 'budget', label: 'Daily budget', icon: Hourglass },
		{ kind: 'friction', label: 'Speed bump', icon: Wind },
		{ kind: 'clean', label: 'Clean a site', icon: Sparkles }
	];
</script>

<aside class="panel">
	<header>
		<Logo size={18} />
		<strong>Pacts</strong>
		<button class="x" aria-label="Close panel" onclick={() => (browser.panelOpen = false)}><X size={16} /></button>
	</header>

	<div class="scroll">
		<div class="stats">
			<div><b class="tabular">{pacts.live.length}</b><span>in force</span></div>
			<div><b class="tabular">{pacts.attemptsToday()}</b><span>blocked today</span></div>
			<div><b class="tabular">{pacts.closed.filter((p) => status(p, clock.now) === 'kept').length}</b><span>kept</span></div>
		</div>

		{#if live.length}
			<h3 class="eyebrow">In force</h3>
			<div class="list">
				{#each live as p (p.id)}<PactRow pact={p} />{/each}
			</div>
		{/if}
		{#if upcoming.length}
			<h3 class="eyebrow">Upcoming</h3>
			<div class="list">
				{#each upcoming as p (p.id)}<PactRow pact={p} />{/each}
			</div>
		{/if}
		{#if !open.length}
			<div class="empty">
				<p><strong>Nothing sealed yet.</strong></p>
				<p>A pact is a promise the browser keeps for you. Once sealed, it can only get stricter.</p>
			</div>
		{/if}

		<h3 class="eyebrow">New pact</h3>
		<div class="quick">
			{#each QUICK as q}
				<button onclick={() => browser.newPact({ kind: q.kind })}>
					<q.icon size={15} />
					{q.label}
				</button>
			{/each}
		</div>
	</div>

	<footer>
		<button class="btn primary" onclick={() => browser.newPact()}><Plus size={16} /> New pact</button>
		<button class="btn ghost" onclick={() => browser.navigate('mast://pacts')}>Open ledger</button>
	</footer>
</aside>

<style>
	.panel {
		display: flex;
		flex-direction: column;
		width: 340px;
		flex: none;
		background: var(--chrome);
		border-left: 1px solid var(--line);
		animation: slide 0.2s ease-out;
	}
	@keyframes slide {
		from {
			transform: translateX(20px);
			opacity: 0;
		}
	}
	header {
		display: flex;
		align-items: center;
		gap: 10px;
		padding: 14px 12px 10px 16px;
	}
	header strong {
		font-size: 15px;
	}
	.x {
		margin-left: auto;
		display: grid;
		place-items: center;
		width: 28px;
		height: 28px;
		border: 0;
		border-radius: 8px;
		background: none;
		color: var(--muted);
		cursor: pointer;
	}
	.x:hover {
		background: var(--raised);
		color: var(--text);
	}
	.scroll {
		flex: 1;
		overflow-y: auto;
		padding: 4px 14px 16px;
	}
	.stats {
		display: grid;
		grid-template-columns: repeat(3, 1fr);
		gap: 8px;
		margin-bottom: 8px;
	}
	.stats div {
		display: grid;
		padding: 10px 12px;
		border-radius: 12px;
		background: var(--chrome-2);
		border: 1px solid var(--line);
	}
	.stats b {
		font-size: 20px;
		font-weight: 700;
	}
	.stats span {
		font-size: 11.5px;
		color: var(--muted);
	}
	h3 {
		margin: 18px 2px 8px;
	}
	.list {
		display: grid;
		gap: 8px;
	}
	.empty {
		padding: 14px;
		border-radius: 12px;
		border: 1px dashed var(--line-2);
		color: var(--muted);
		font-size: 13px;
	}
	.empty p {
		margin: 0 0 4px;
	}
	.empty strong {
		color: var(--text);
	}
	.quick {
		display: grid;
		grid-template-columns: 1fr 1fr;
		gap: 6px;
	}
	.quick button {
		display: flex;
		align-items: center;
		gap: 8px;
		height: 36px;
		padding: 0 10px;
		border: 1px solid var(--line);
		border-radius: 10px;
		background: var(--chrome-2);
		font-size: 12.5px;
		cursor: pointer;
	}
	.quick button:hover {
		background: var(--raised);
	}
	.quick button :global(svg) {
		color: var(--muted);
	}
	footer {
		display: flex;
		gap: 8px;
		padding: 12px 14px;
		border-top: 1px solid var(--line);
	}
	footer .btn.primary {
		flex: 1;
	}
</style>
