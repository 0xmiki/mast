<script lang="ts">
	import { ChevronRight } from '@lucide/svelte';
	import { browser } from '#lib/mast/browser.svelte.ts';
	import { pacts } from '#lib/mast/pacts.svelte.ts';
	import { clock } from '#lib/mast/clock.svelte.ts';
	import { enforcing, formatDuration, KIND_LABELS, listPhrase, progress, ruleLine, status } from '#lib/mast/engine.ts';
	import { KIND_COLOR, STATUS_CHIP } from '#lib/mast/ui.ts';
	import type { Pact } from '#lib/mast/types.ts';
	import KindIcon from './KindIcon.svelte';

	let { pact }: { pact: Pact } = $props();

	const now = $derived(clock.now);
	const st = $derived(status(pact, now));
	const chip = $derived(STATUS_CHIP[st]);
	const used = $derived(pacts.usedToday(pact));
	const hot = $derived(enforcing(pact, now));
	const budgetFill = $derived(
		pact.rule.kind === 'budget' ? Math.min(1, used / (pact.rule.minutesPerDay * 60_000 || 1)) : 0
	);
</script>

<a
	class="row"
	class:hot
	href="mast://pacts/{pact.id}"
	style:--k={KIND_COLOR[pact.rule.kind]}
	onclick={(e) => {
		e.preventDefault();
		browser.navigate(`mast://pacts/${pact.id}`);
	}}
>
	<span class="icon"><KindIcon kind={pact.rule.kind} size={16} /></span>
	<span class="main">
		<span class="top">
			<strong>{KIND_LABELS[pact.rule.kind]}</strong>
			<span class="chip {chip.tone}">
				{#if st === 'grace'}
					<span class="tabular">{chip.label} · {formatDuration(pact.graceEndsAt - now)}</span>
				{:else}
					{chip.label}
				{/if}
			</span>
		</span>
		<span class="sites">{listPhrase(pact.sites)}</span>
		<span class="line tabular">{ruleLine(pact, now, used)}</span>
		{#if pact.rule.kind === 'budget' && st !== 'kept' && st !== 'released'}
			<span class="meter"><span style:width="{budgetFill * 100}%"></span></span>
		{:else if st !== 'kept' && st !== 'released'}
			<span class="term"><span style:width="{progress(pact, now) * 100}%"></span></span>
		{/if}
	</span>
	<ChevronRight size={16} />
</a>

<style>
	.row {
		display: flex;
		align-items: flex-start;
		gap: 12px;
		padding: 12px;
		border-radius: 12px;
		color: var(--text);
		text-decoration: none;
		border: 1px solid var(--line);
		background: var(--chrome-2);
		transition:
			background 0.15s,
			border-color 0.15s;
	}
	.row:hover {
		background: var(--raised);
		border-color: var(--line-2);
	}
	.row > :global(svg) {
		color: var(--faint);
		align-self: center;
		flex: none;
	}
	.icon {
		display: grid;
		place-items: center;
		width: 32px;
		height: 32px;
		border-radius: 10px;
		background: color-mix(in srgb, var(--k) 14%, transparent);
		color: var(--k);
		flex: none;
	}
	.hot .icon {
		box-shadow: 0 0 0 1px color-mix(in srgb, var(--k) 40%, transparent);
	}
	.main {
		display: grid;
		gap: 3px;
		flex: 1;
		min-width: 0;
	}
	.top {
		display: flex;
		align-items: center;
		justify-content: space-between;
		gap: 8px;
	}
	.top strong {
		font-size: 13.5px;
	}
	.top .chip {
		height: 20px;
		font-size: 11px;
		padding: 0 8px;
	}
	.sites {
		font-size: 12.5px;
		color: var(--text);
		opacity: 0.85;
		overflow: hidden;
		text-overflow: ellipsis;
		white-space: nowrap;
	}
	.line {
		font-size: 12px;
		color: var(--muted);
	}
	.meter,
	.term {
		height: 4px;
		margin-top: 6px;
		border-radius: 99px;
		background: var(--raised-2);
		overflow: hidden;
	}
	.meter span,
	.term span {
		display: block;
		height: 100%;
		border-radius: inherit;
		background: var(--k);
	}
	.term span {
		opacity: 0.6;
	}
</style>
