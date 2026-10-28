<script lang="ts">
	import { Search, Lock } from '@lucide/svelte';
	import { browser } from '#lib/mast/browser.svelte.ts';
	import { focus } from '#lib/mast/focus.svelte.ts';
	import { clock } from '#lib/mast/clock.svelte.ts';
	import { formatDuration } from '#lib/mast/engine.ts';
	import { titleFor } from '#lib/mast/url.ts';
	import Favicon from '../chrome/Favicon.svelte';
	import StartControl from '../StartControl.svelte';

	let q = $state('');
	const now = $derived(new Date(clock.now));
	const time = $derived(now.toLocaleTimeString([], { hour: '2-digit', minute: '2-digit' }));
	const date = $derived(now.toLocaleDateString([], { weekday: 'long', month: 'long', day: 'numeric' }));

	const SHORTCUTS = [
		'https://github.com/mast-browser/mast',
		'https://en.wikipedia.org/wiki/Ulysses_pact',
		'https://youtube.com/',
		'https://x.com/',
		'https://reddit.com/',
		'https://news.ycombinator.com/'
	];
</script>

<div class="ntp" class:running={!!focus.active}>
	{#if !focus.active}
		<div class="top">
			<div class="time tabular">{time}</div>
			<div class="date">{date}</div>
		</div>
	{/if}

	<div class="hero"><StartControl /></div>

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
			{@const blocked = focus.blocks(href)}
			<a class="tile" class:blocked {href}>
				<span class="fav">
					<Favicon {href} size={26} />
					{#if blocked}<span class="badge"><Lock size={11} strokeWidth={2.6} /></span>{/if}
				</span>
				<span class="name">{titleFor(href)}</span>
			</a>
		{/each}
	</div>

	{#if focus.focusedToday > 0 || focus.attemptsToday > 0}
		<div class="today">
			<span><b class="tabular">{formatDuration(focus.focusedToday)}</b> deep work today</span>
			<span><b class="tabular">{focus.attemptsToday}</b> distractions blocked</span>
		</div>
	{/if}
</div>

<style>
	.ntp {
		display: flex;
		flex-direction: column;
		align-items: center;
		min-height: 100%;
		padding: 8vh 24px 48px;
		background: radial-gradient(70% 40% at 50% 0%, rgb(255 106 61 / 0.06), transparent 70%), var(--bg);
	}
	.ntp.running {
		padding-top: 12vh;
		background: radial-gradient(70% 50% at 50% 0%, rgb(255 106 61 / 0.13), transparent 70%), var(--bg);
	}
	.top {
		text-align: center;
	}
	.time {
		font-size: 56px;
		font-weight: 650;
		letter-spacing: -0.04em;
		line-height: 1;
	}
	.date {
		margin-top: 10px;
		color: var(--muted);
	}
	.hero {
		margin-top: 44px;
	}
	.running .hero {
		margin-top: 0;
	}
	.search {
		display: flex;
		align-items: center;
		gap: 12px;
		width: min(560px, 100%);
		height: 46px;
		margin-top: 52px;
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
		grid-template-columns: repeat(6, 92px);
		gap: 8px;
		margin-top: 24px;
	}
	.tile {
		display: flex;
		flex-direction: column;
		align-items: center;
		gap: 8px;
		padding: 12px 6px 10px;
		border-radius: 14px;
		color: var(--text);
		text-decoration: none;
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
	.tile.blocked .fav > :global(:first-child) {
		filter: grayscale(1);
		opacity: 0.4;
	}
	.tile.blocked .name {
		color: var(--faint);
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
		background: var(--ember);
		color: #1a0b05;
	}
	.name {
		font-size: 12.5px;
		max-width: 100%;
		overflow: hidden;
		text-overflow: ellipsis;
		white-space: nowrap;
	}
	.today {
		display: flex;
		gap: 28px;
		margin-top: 36px;
		color: var(--muted);
		font-size: 13px;
	}
	.today b {
		color: var(--text);
		font-weight: 650;
	}
</style>
