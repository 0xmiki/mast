<script lang="ts">
	import { Search } from '@lucide/svelte';
	import { browser } from '#lib/mast/browser.svelte.ts';
	import { blocks } from '#lib/mast/blocks.svelte.ts';
	import { clock } from '#lib/mast/clock.svelte.ts';
	import { DAY, startOfDay } from '#lib/mast/engine.ts';
	import BlockPanel from '../BlockPanel.svelte';

	let q = $state('');
	const now = $derived(new Date(clock.now));
	const time = $derived(now.toLocaleTimeString([], { hour: '2-digit', minute: '2-digit' }));
	const date = $derived(now.toLocaleDateString([], { weekday: 'long', month: 'long', day: 'numeric' }));

	/** The logbook: distractions stopped per day, last seven days. */
	const week = $derived.by(() => {
		const today = startOfDay(clock.now);
		const days = Array.from({ length: 7 }, (_, i) => {
			const from = today - (6 - i) * DAY;
			const n = blocks.stoppedBetween(from, from + DAY);
			return { label: new Date(from).toLocaleDateString([], { weekday: 'narrow' }), n, today: i === 6 };
		});
		const max = Math.max(10, ...days.map((d) => d.n));
		return days.map((d) => ({ ...d, h: d.n / max }));
	});

	/** Etched sea: horizontal strokes that crowd together toward the horizon. */
	const SEA = Array.from({ length: 18 }, (_, i) => {
		const t = (i + 1) / 18;
		const y = Math.pow(t, 2.2) * 580;
		let seed = (i + 3) * 9301;
		const rnd = () => ((seed = (seed * 49297 + 233280) % 233280) / 233280);
		const dash = Array.from({ length: 10 }, () => [Math.round(20 + rnd() * 180 * (1.4 - t)), Math.round(30 + rnd() * 160 * t)]).flat().join(' ');
		return { y, dash, o: 0.3 - t * 0.2, off: Math.round(rnd() * 400) };
	});
</script>

<div class="ntp">
	<svg class="sea" viewBox="0 0 1600 600" preserveAspectRatio="xMidYMin slice" aria-hidden="true">
		<line class="sky-line" x1="0" x2="1600" y1="0.5" y2="0.5" />
		{#each SEA as l}
			<line x1="0" x2="1600" y1={l.y + 0.5} y2={l.y + 0.5} stroke-dasharray={l.dash} stroke-dashoffset={l.off} style:opacity={l.o} />
		{/each}
	</svg>

	<!-- A lone sail on the horizon, and its broken reflection. -->
	<svg class="boat" viewBox="-12 -22 24 40" aria-hidden="true">
		<path d="M-2 -14V0H-9.4Q-8.6 -8.5 -2 -14Z" />
		<path d="M1 -18.5V0H9.4Q8.7 -11.5 1 -18.5Z" />
		<line x1="-9" x2="10" y1="4" y2="4" />
		<line x1="-6" x2="7" y1="8.5" y2="8.5" />
		<line x1="-3" x2="3" y1="13" y2="13" />
	</svg>

	<header>
		<div class="when">
			<span class="mono">{time}</span>
			<span class="date">{date}</span>
		</div>
		<form
			class="search"
			onsubmit={(e) => {
				e.preventDefault();
				if (q.trim()) browser.navigate(q);
			}}
		>
			<Search size={15} strokeWidth={1.75} />
			<input bind:value={q} placeholder="Search Google" spellcheck="false" />
		</form>
		<div class="spacer"></div>
	</header>

	<main><BlockPanel /></main>

	<footer>
		<div class="log" aria-label="Focus, last 7 days">
			{#each week as d}
				<div class="day" class:today={d.today} title="{d.n} stopped">
					<span class="bar"><span style:height="{Math.max(d.h * 100, 4)}%" class:empty={!d.n}></span></span>
					<span class="mono">{d.label}</span>
				</div>
			{/each}
		</div>
		<p>
			<span class="mono">{blocks.stoppedToday}</span> stopped today&ensp;·&ensp;<span class="mono">{blocks.active.length}</span>
			{blocks.active.length === 1 ? 'site' : 'sites'} blocked
		</p>
	</footer>
</div>

<style>
	.ntp {
		position: relative;
		display: grid;
		grid-template-rows: auto 1fr auto;
		min-height: 100%;
		padding: 18px 28px 26px;
		overflow: hidden;
		background: var(--bg);
		isolation: isolate;
	}
	/* Horizon sits at 82% of the page; everything below is etched water. */
	.sea {
		position: absolute;
		left: 0;
		right: 0;
		top: 82%;
		width: 100%;
		height: 18%;
		z-index: -1;
		overflow: visible;
		mask-image: radial-gradient(42% 90% at 50% 0%, #000 0, rgb(0 0 0 / 0.5) 45%, transparent 100%);
		stroke: #fff;
		stroke-width: 1;
	}
	.sea .sky-line {
		stroke: rgb(255 255 255 / 0.5);
		stroke-dasharray: none;
	}
	.ntp::before {
		content: '';
		position: absolute;
		inset: 0;
		z-index: -1;
		background: radial-gradient(60% 24% at 50% 82%, rgb(255 255 255 / 0.025), transparent 70%);
		transition: background 1s;
	}
	.boat {
		position: absolute;
		left: 61%;
		top: 82%;
		width: 26px;
		height: 44px;
		margin: -24px 0 0 -13px;
		z-index: -1;
		stroke: #fff;
		stroke-width: 0.8;
	}
	.boat path {
		fill: #fff;
		stroke: none;
		opacity: 0.6;
	}
	.boat line {
		opacity: 0.25;
	}

	header {
		display: grid;
		grid-template-columns: 1fr minmax(0, 460px) 1fr;
		align-items: center;
		gap: 24px;
	}
	.when {
		display: flex;
		align-items: baseline;
		gap: 12px;
		color: var(--faint);
		font-size: 12.5px;
		white-space: nowrap;
	}
	.when .mono {
		color: var(--muted);
		font-size: 13px;
	}
	.search {
		display: flex;
		align-items: center;
		gap: 10px;
		height: 36px;
		padding: 0 12px;
		border-radius: var(--r);
		background: var(--chrome);
		box-shadow: inset 0 0 0 1px var(--line);
		color: var(--faint);
	}
	.search:focus-within {
		box-shadow: inset 0 0 0 1px var(--line-3);
	}
	.search input {
		flex: 1;
		min-width: 0;
		border: 0;
		outline: 0;
		background: none;
		color: var(--text);
		font-size: 13.5px;
	}
	.search input::placeholder {
		color: var(--faint);
	}

	main {
		display: grid;
		justify-items: center;
		align-content: start;
		padding: 7vh 0 12vh;
	}

	footer {
		display: flex;
		align-items: flex-end;
		gap: 22px;
		color: var(--faint);
		font-size: 12.5px;
	}
	footer p {
		margin: 0 0 1px;
	}
	footer p .mono {
		color: var(--muted);
	}
	.log {
		display: flex;
		gap: 9px;
	}
	.day {
		display: grid;
		justify-items: center;
		gap: 6px;
	}
	.bar {
		display: flex;
		align-items: flex-end;
		width: 3px;
		height: 30px;
	}
	.bar span {
		width: 100%;
		background: var(--line-3);
	}
	.bar span.empty {
		background: var(--line);
	}
	.today .bar span:not(.empty) {
		background: var(--text);
	}
	.day .mono {
		font-size: 10px;
	}
	.today .mono {
		color: var(--text);
	}
</style>
