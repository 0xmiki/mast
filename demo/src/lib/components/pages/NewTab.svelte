<script lang="ts">
	import { Search, Lock, Wind, Sparkles, Hourglass, Ban, Crosshair, ArrowRight } from '@lucide/svelte';
	import { browser } from '#lib/mast/browser.svelte.ts';
	import { pacts } from '#lib/mast/pacts.svelte.ts';
	import { clock } from '#lib/mast/clock.svelte.ts';
	import { formatDuration, formatWhen } from '#lib/mast/engine.ts';
	import { titleFor } from '#lib/mast/url.ts';
	import type { PactKind } from '#lib/mast/types.ts';
	import Favicon from '../chrome/Favicon.svelte';
	import PactRow from '../PactRow.svelte';

	let q = $state('');
	const now = $derived(new Date(clock.now));
	const time = $derived(now.toLocaleTimeString([], { hour: '2-digit', minute: '2-digit' }));
	const date = $derived(now.toLocaleDateString([], { weekday: 'long', month: 'long', day: 'numeric' }));
	const greeting = $derived(now.getHours() < 12 ? 'Good morning' : now.getHours() < 18 ? 'Good afternoon' : 'Good evening');
	const focus = $derived(pacts.focus);

	const SHORTCUTS = [
		'https://youtube.com/',
		'https://x.com/',
		'https://reddit.com/',
		'https://news.ycombinator.com/',
		'https://github.com/mast-browser/mast',
		'https://en.wikipedia.org/wiki/Ulysses_pact'
	];

	const TEMPLATES: { kind: PactKind; title: string; text: string; icon: typeof Ban }[] = [
		{ kind: 'block', title: 'Guard work hours', text: 'Social sites sealed 9–5 on weekdays, for a month.', icon: Ban },
		{ kind: 'focus', title: 'Deep focus, now', text: 'For 50 minutes, only the sites your work needs.', icon: Crosshair },
		{ kind: 'budget', title: 'A daily allowance', text: '20 minutes a day of feeds. Then it’s gone till midnight.', icon: Hourglass }
	];

	const SEEDS: Record<string, object> = {
		block: { sites: ['youtube.com', 'x.com', 'reddit.com'] },
		focus: { sites: ['github.com', 'developer.mozilla.org'] },
		budget: { sites: ['x.com', 'reddit.com', 'news.ycombinator.com'] }
	};
</script>

<div class="ntp">
	<div class="top">
		<div class="time tabular">{time}</div>
		<div class="date">{greeting} · {date}</div>
	</div>

	<form
		class="search"
		onsubmit={(e) => {
			e.preventDefault();
			if (q.trim()) browser.navigate(q);
		}}
	>
		<Search size={18} />
		<input bind:value={q} placeholder="Search or type a URL" spellcheck="false" />
	</form>

	<div class="tiles">
		{#each SHORTCUTS as href}
			{@const v = pacts.verdict(href)}
			{@const sealed = v.kind === 'block' || v.kind === 'focus-block' || v.kind === 'budget-out'}
			<a class="tile" class:sealed {href}>
				<span class="fav">
					<Favicon {href} size={26} />
					{#if sealed}
						<span class="badge ember"><Lock size={11} strokeWidth={2.6} /></span>
					{:else if v.kind === 'friction'}
						<span class="badge teal"><Wind size={11} strokeWidth={2.6} /></span>
					{:else if v.kind === 'allow' && v.budget}
						<span class="badge warn"><Hourglass size={11} strokeWidth={2.6} /></span>
					{:else if v.kind === 'allow' && v.clean.length}
						<span class="badge good"><Sparkles size={11} strokeWidth={2.6} /></span>
					{/if}
				</span>
				<span class="name">{titleFor(href)}</span>
				<span class="sub tabular">
					{#if 'until' in v}
						until {formatWhen(v.until, clock.now).replace('today ', '')}
					{:else if v.kind === 'allow' && v.budget}
						{formatDuration(v.budget.remainingMs)} left
					{:else if v.kind === 'friction'}
						{v.waitSeconds}s pause
					{:else}
						&nbsp;
					{/if}
				</span>
			</a>
		{/each}
	</div>

	{#if focus}
		<a class="focus card" href="mast://pacts/{focus.id}">
			<Crosshair size={20} />
			<div>
				<strong class="tabular">Focus · {formatDuration(focus.endsAt - clock.now)} left</strong>
				<span>Only {focus.sites.join(', ')} open until {formatWhen(focus.endsAt, clock.now)}.</span>
			</div>
		</a>
	{/if}

	{#if pacts.open.length}
		<section>
			<div class="head">
				<h2 class="eyebrow">Your pacts</h2>
				<a href="mast://pacts">Ledger <ArrowRight size={13} /></a>
			</div>
			<div class="grid">
				{#each pacts.open.slice(0, 4) as p (p.id)}<PactRow pact={p} />{/each}
			</div>
		</section>
	{:else}
		<section class="hero">
			<h2>Tie yourself to the mast.</h2>
			<p>
				Make a promise to future you, and Mast keeps it. Once sealed, a pact can’t be paused,
				deleted or loosened. Not from settings, not from incognito, not by you at 1am.
			</p>
			<div class="templates">
				{#each TEMPLATES as t}
					<button class="tpl" onclick={() => browser.newPact({ kind: t.kind, ...SEEDS[t.kind] })}>
						<t.icon size={18} />
						<strong>{t.title}</strong>
						<span>{t.text}</span>
					</button>
				{/each}
			</div>
		</section>
	{/if}
</div>

<style>
	.ntp {
		display: flex;
		flex-direction: column;
		align-items: center;
		min-height: 100%;
		padding: 9vh 24px 48px;
		background: radial-gradient(70% 40% at 50% 0%, rgb(255 106 61 / 0.07), transparent 70%), var(--bg);
	}
	.top {
		text-align: center;
	}
	.time {
		font-size: 64px;
		font-weight: 650;
		letter-spacing: -0.04em;
		line-height: 1;
	}
	.date {
		margin-top: 10px;
		color: var(--muted);
	}
	.search {
		display: flex;
		align-items: center;
		gap: 12px;
		width: min(580px, 100%);
		height: 48px;
		margin-top: 32px;
		padding: 0 18px;
		border-radius: 999px;
		background: var(--chrome-2);
		border: 1px solid var(--line-2);
		color: var(--muted);
	}
	.search:focus-within {
		border-color: var(--ember);
		box-shadow: 0 0 0 3px var(--ember-soft);
	}
	.search input {
		flex: 1;
		border: 0;
		outline: 0;
		background: none;
		color: var(--text);
		font-size: 15px;
	}
	.tiles {
		display: grid;
		grid-template-columns: repeat(6, 96px);
		gap: 10px;
		margin-top: 28px;
	}
	.tile {
		display: flex;
		flex-direction: column;
		align-items: center;
		gap: 6px;
		padding: 14px 6px 10px;
		border-radius: 14px;
		color: var(--text);
		text-decoration: none;
		transition: background 0.15s;
	}
	.tile:hover {
		background: var(--chrome-2);
	}
	.fav {
		position: relative;
		display: grid;
		place-items: center;
		width: 48px;
		height: 48px;
		border-radius: 14px;
		background: var(--chrome-2);
		border: 1px solid var(--line);
	}
	.tile.sealed .fav > :global(:first-child) {
		filter: grayscale(1);
		opacity: 0.45;
	}
	.badge {
		position: absolute;
		right: -5px;
		bottom: -5px;
		display: grid;
		place-items: center;
		width: 20px;
		height: 20px;
		border-radius: 50%;
		border: 2px solid var(--bg);
	}
	.badge.ember {
		background: var(--ember);
		color: #1a0b05;
	}
	.badge.teal {
		background: #4cc9d9;
		color: #041417;
	}
	.badge.warn {
		background: var(--warn);
		color: #1d1303;
	}
	.badge.good {
		background: var(--good);
		color: #03170d;
	}
	.name {
		font-size: 12.5px;
		max-width: 100%;
		overflow: hidden;
		text-overflow: ellipsis;
		white-space: nowrap;
	}
	.sub {
		font-size: 11px;
		color: var(--faint);
		white-space: nowrap;
	}
	.tile.sealed .sub {
		color: var(--ember-2);
	}
	.focus {
		display: flex;
		align-items: center;
		gap: 14px;
		width: min(640px, 100%);
		margin-top: 28px;
		padding: 16px 18px;
		color: var(--text);
		text-decoration: none;
		border-color: color-mix(in srgb, var(--focus) 35%, transparent);
		background: var(--focus-soft);
	}
	.focus :global(svg) {
		color: var(--focus);
	}
	.focus div {
		display: grid;
	}
	.focus span {
		color: var(--muted);
		font-size: 13px;
	}
	section {
		width: min(760px, 100%);
		margin-top: 40px;
	}
	.head {
		display: flex;
		align-items: center;
		justify-content: space-between;
		margin-bottom: 10px;
	}
	.head h2 {
		margin: 0;
	}
	.head a {
		display: inline-flex;
		align-items: center;
		gap: 4px;
		color: var(--muted);
		font-size: 12.5px;
		text-decoration: none;
	}
	.head a:hover {
		color: var(--text);
	}
	.grid {
		display: grid;
		grid-template-columns: 1fr 1fr;
		gap: 10px;
	}
	.hero {
		text-align: center;
	}
	.hero h2 {
		margin: 0;
		font-size: 26px;
		letter-spacing: -0.02em;
	}
	.hero p {
		max-width: 540px;
		margin: 10px auto 0;
		color: var(--muted);
	}
	.templates {
		display: grid;
		grid-template-columns: repeat(3, 1fr);
		gap: 10px;
		margin-top: 24px;
	}
	.tpl {
		align-content: start;
		display: grid;
		gap: 6px;
		justify-items: start;
		padding: 16px;
		border-radius: 14px;
		border: 1px solid var(--line);
		background: var(--chrome);
		text-align: left;
		cursor: pointer;
		transition:
			border-color 0.15s,
			transform 0.15s;
	}
	.tpl:hover {
		border-color: var(--ember);
		transform: translateY(-2px);
	}
	.tpl :global(svg) {
		color: var(--ember);
	}
	.tpl span {
		color: var(--muted);
		font-size: 12.5px;
	}
</style>
