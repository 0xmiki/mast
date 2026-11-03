<script lang="ts">
	import { CornerDownLeft } from '@lucide/svelte';
	import { browser } from '#lib/mast/browser.svelte.ts';
	import { blocks, DURATIONS, untilFor, type DurationId } from '#lib/mast/blocks.svelte.ts';
	import { clock } from '#lib/mast/clock.svelte.ts';
	import { UNDO_MS, untilText } from '#lib/mast/engine.ts';

	let {
		domains,
		what,
		compact = false,
		onblocked
	}: { domains: string[]; what: string; compact?: boolean; onblocked?: () => void } = $props();

	let index = $state(Math.max(0, DURATIONS.findIndex((d) => d.id === blocks.lastDuration)));
	const duration = $derived(DURATIONS[index]);
	const forever = $derived(duration.id === 'forever');
	const pos = (i: number) => (i / (DURATIONS.length - 1)) * 100;
	let ruler = $state<HTMLDivElement>();
	let dragging = false;

	function pick(e: PointerEvent) {
		const r = ruler!.getBoundingClientRect();
		index = Math.round(Math.max(0, Math.min(1, (e.clientX - r.left) / r.width)) * (DURATIONS.length - 1));
	}
	function step(d: number) {
		index = Math.max(0, Math.min(DURATIONS.length - 1, index + d));
	}

	export function block() {
		if (!domains.length) return;
		const until = untilFor(duration.id as DurationId, clock.now);
		const n = blocks.block(domains, until);
		blocks.setDuration(duration.id);
		if (!n) browser.toast(`Already blocked at least that long.`);
		else
			browser.toast(
				`Blocked ${n === 1 && domains.length === 1 ? domains[0] : `${n} sites`} ${untilText(until, clock.now)}.`,
				'lock',
				{ label: 'Undo', run: () => blocks.undo() && browser.toast('Undone.') },
				UNDO_MS
			);
		onblocked?.();
	}

	function onkeydown(e: KeyboardEvent) {
		if (e.key === 'ArrowRight' || e.key === 'ArrowUp') step(1);
		else if (e.key === 'ArrowLeft' || e.key === 'ArrowDown') step(-1);
		else if (e.key === 'Enter') block();
		else return;
		e.preventDefault();
	}
</script>

<div class="act" class:compact>
	<div
		class="ruler"
		bind:this={ruler}
		role="slider"
		tabindex="0"
		aria-label="How long"
		aria-valuemin={0}
		aria-valuemax={DURATIONS.length - 1}
		aria-valuenow={index}
		aria-valuetext={duration.label}
		{onkeydown}
		onpointerdown={(e) => {
			dragging = true;
			ruler!.setPointerCapture(e.pointerId);
			pick(e);
		}}
		onpointermove={(e) => dragging && pick(e)}
		onpointerup={() => (dragging = false)}
		onwheel={(e) => {
			e.preventDefault();
			step(e.deltaY > 0 || e.deltaX > 0 ? 1 : -1);
		}}
	>
		<span class="base"></span>
		<span class="forever-zone" style:left="{pos(DURATIONS.length - 2)}%"></span>
		{#each DURATIONS as d, i}
			<span class="tick major" class:passed={i <= index} style:left="{pos(i)}%"></span>
			{#if i < DURATIONS.length - 1}
				{#each { length: 3 } as _, k}
					<span class="tick" style:left="{pos(i) + ((k + 1) / 4) * pos(1)}%"></span>
				{/each}
			{/if}
			<span class="tlabel mono" class:on={i === index} class:inf={d.id === 'forever'} style:left="{pos(i)}%">{d.short}</span>
		{/each}
		<span class="needle" class:forever style:left="{pos(index)}%"></span>
	</div>

	<button class="btn primary go" disabled={!domains.length} onclick={block}>
		Block {what}
		{duration.label}
		{#if !compact}<span class="kbd"><CornerDownLeft size={12} strokeWidth={2.25} /></span>{/if}
	</button>
	{#if forever}
		<p class="warn">There is no way to unblock after the first 10 seconds.</p>
	{/if}
</div>

<style>
	.act {
		display: flex;
		flex-wrap: wrap;
		align-items: center;
		gap: 10px 26px;
	}
	.ruler {
		position: relative;
		flex: 1;
		min-width: 260px;
		height: 40px;
		margin: 0 12px;
		cursor: ew-resize;
		touch-action: none;
		outline: none;
	}
	.ruler::before {
		content: '';
		position: absolute;
		inset: -4px -12px;
		border-radius: var(--r);
	}
	.ruler:focus-visible::before {
		box-shadow: inset 0 0 0 1px var(--line-2);
	}
	.base {
		position: absolute;
		left: 0;
		right: 0;
		top: 14px;
		height: 1px;
		background: var(--line-2);
	}
	/* The last stretch of the scale, toward forever, is drawn in ember. */
	.forever-zone {
		position: absolute;
		right: 0;
		top: 14px;
		height: 1px;
		background: linear-gradient(90deg, var(--line-2), rgb(255 106 61 / 0.6));
	}
	.tick {
		position: absolute;
		top: 10px;
		width: 1px;
		height: 4px;
		background: var(--line-2);
		transform: translateX(-0.5px);
	}
	.tick.major {
		top: 6px;
		height: 8px;
		background: var(--line-3);
	}
	.tick.major.passed {
		background: var(--muted);
	}
	.tlabel {
		position: absolute;
		top: 21px;
		font-size: 10.5px;
		color: var(--faint);
		transform: translateX(-50%);
		white-space: nowrap;
		transition: color 0.15s;
	}
	.tlabel.on {
		color: var(--text);
	}
	.tlabel.inf {
		font-size: 14px;
		top: 17px;
	}
	.tlabel.inf.on {
		color: var(--ember-2);
	}
	.needle {
		position: absolute;
		top: 0;
		width: 2px;
		height: 17px;
		margin-left: -1px;
		background: var(--text);
		transition: left 0.18s cubic-bezier(0.3, 0.7, 0.2, 1);
	}
	.needle::before {
		content: '';
		position: absolute;
		top: -5px;
		left: -4px;
		border: 5px solid transparent;
		border-top-color: currentColor;
		color: var(--text);
	}
	.needle.forever {
		background: var(--ember);
	}
	.needle.forever::before {
		color: var(--ember);
	}
	.go {
		height: 42px;
		padding: 0 14px 0 18px;
		font-size: 14px;
		font-weight: 650;
		gap: 6px;
	}
	.kbd {
		display: grid;
		place-items: center;
		width: 22px;
		height: 20px;
		margin-left: 6px;
		border-radius: 2px;
		background: rgb(0 0 0 / 0.14);
	}
	.warn {
		flex-basis: 100%;
		margin: 0;
		font-size: 12px;
		text-align: right;
		color: var(--ember-2);
	}

	.compact {
		flex-direction: column;
		align-items: stretch;
		gap: 14px;
	}
	.compact .ruler {
		flex: none;
		min-width: 0;
		margin-top: 8px;
	}
	.compact .go {
		width: 100%;
		justify-content: center;
	}
	.compact .warn {
		text-align: center;
	}
</style>
