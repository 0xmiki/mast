<script lang="ts">
	import { Plus, Undo2 } from '@lucide/svelte';
	import { browser } from '#lib/mast/browser.svelte.ts';
	import { focus } from '#lib/mast/focus.svelte.ts';
	import { clock } from '#lib/mast/clock.svelte.ts';
	import { UNDO_MS, clockTime, countdown } from '#lib/mast/engine.ts';

	let { compact = false }: { compact?: boolean } = $props();

	const LENGTHS = [25, 50, 90, 120, 180, 240];
	let minutes = $state(focus.lastMinutes);
	const active = $derived(focus.active);

	function label(m: number) {
		return m < 60 ? `${m}m` : m % 60 ? `${Math.floor(m / 60)}h ${m % 60}m` : `${m / 60}h`;
	}

	function start() {
		focus.start(minutes);
		browser.toast(`Blocking ${focus.blocklist.length} sites until ${clockTime(focus.active!.endsAt)}.`, 'lock', {
			label: 'Undo',
			run: () => focus.undo() && browser.toast('Cancelled.')
		}, UNDO_MS);
	}
</script>

<div class="ctl" class:compact class:running={!!active}>
	{#if active}
		<p class="eyebrow">Deep work</p>
		<div class="time tabular">{countdown(active.endsAt - clock.now)}</div>
		<p class="sub">
			Until {clockTime(active.endsAt)} · {focus.blocklist.length} sites blocked
		</p>
		<div class="row">
			<button class="btn" onclick={() => focus.extend(30)}><Plus size={15} /> 30 min</button>
			{#if focus.canUndo}
				<button class="btn ghost" onclick={() => focus.undo()}>
					<Undo2 size={15} /> Undo · {Math.ceil((UNDO_MS - (clock.now - active.startedAt)) / 1000)}s
				</button>
			{/if}
		</div>
		{#if !focus.canUndo}<p class="note">There’s no stop button. That’s the point.</p>{/if}
	{:else}
		<p class="eyebrow">Block distractions for</p>
		<div class="lengths">
			{#each LENGTHS as m}
				<button class:on={minutes === m} onclick={() => (minutes = m)}>{label(m)}</button>
			{/each}
		</div>
		<button class="btn primary go" onclick={start}>Start · {label(minutes)}</button>
		<p class="note">
			{focus.blocklist.length} sites ·
			<a href="mast://blocklist" onclick={(e) => { e.preventDefault(); browser.navigate('mast://blocklist'); }}>see the list</a>
		</p>
	{/if}
</div>

<style>
	.ctl {
		display: flex;
		flex-direction: column;
		align-items: center;
		text-align: center;
	}
	.eyebrow {
		margin: 0 0 14px;
	}
	.running .eyebrow {
		color: var(--ember-2);
	}
	.lengths {
		display: flex;
		flex-wrap: wrap;
		justify-content: center;
		gap: 4px;
		padding: 4px;
		border-radius: 14px;
		background: var(--chrome);
		border: 1px solid var(--line);
	}
	.lengths button {
		min-width: 64px;
		height: 38px;
		padding: 0 14px;
		border: 0;
		border-radius: 10px;
		background: none;
		color: var(--muted);
		font-weight: 600;
		cursor: pointer;
	}
	.lengths button:hover {
		color: var(--text);
	}
	.lengths button.on {
		background: var(--raised-2);
		color: var(--text);
		box-shadow: inset 0 0 0 1px var(--line-2);
	}
	.go {
		width: 100%;
		max-width: 320px;
		height: 52px;
		margin-top: 16px;
		border-radius: 14px;
		font-size: 16px;
		font-weight: 700;
		box-shadow: 0 10px 40px rgb(255 106 61 / 0.25);
	}
	.time {
		font-family: var(--mono);
		font-size: 88px;
		font-weight: 600;
		line-height: 1;
		letter-spacing: -0.03em;
	}
	.sub {
		margin: 14px 0 0;
		color: var(--muted);
	}
	.row {
		display: flex;
		gap: 8px;
		margin-top: 22px;
	}
	.note {
		margin: 14px 0 0;
		font-size: 12.5px;
		color: var(--faint);
	}
	.note a {
		color: var(--muted);
	}
	.note a:hover {
		color: var(--text);
	}

	.compact .eyebrow {
		margin-bottom: 10px;
	}
	.compact .lengths button {
		min-width: 0;
		height: 32px;
		padding: 0 10px;
		font-size: 12.5px;
	}
	.compact .go {
		height: 42px;
		font-size: 14px;
		margin-top: 12px;
	}
	.compact .time {
		font-size: 44px;
	}
	.compact .row {
		margin-top: 14px;
	}
</style>
