<script lang="ts">
	import { Lock, Wind, Hourglass, Sparkles, ShieldCheck, Search } from '@lucide/svelte';
	import { browser } from '#lib/mast/browser.svelte.ts';
	import { pacts } from '#lib/mast/pacts.svelte.ts';
	import { clock } from '#lib/mast/clock.svelte.ts';
	import { formatDuration, formatWhen } from '#lib/mast/engine.ts';
	import { displayUrl, hostOf, isInternal, normalizeInput } from '#lib/mast/url.ts';
	import Logo from '../Logo.svelte';
	import Favicon from './Favicon.svelte';
	import SiteStatus from './SiteStatus.svelte';

	let input: HTMLInputElement;
	let focused = $state(false);
	let text = $state('');
	let statusOpen = $state(false);
	let selected = $state(0);

	const url = $derived(browser.url);
	const verdict = $derived(pacts.verdict(url));
	const internal = $derived(isInternal(url));
	const sealed = $derived(verdict.kind === 'block' || verdict.kind === 'focus-block' || verdict.kind === 'budget-out');

	const KNOWN = [
		'youtube.com',
		'x.com',
		'reddit.com',
		'news.ycombinator.com',
		'github.com',
		'github.com/mast-browser/mast',
		'en.wikipedia.org/wiki/Ulysses_pact',
		'mast://pacts',
		'mast://pacts/new',
		'mast://settings',
		'mast://history'
	];

	const suggestions = $derived.by(() => {
		const q = text.trim().toLowerCase();
		if (!focused || !q || q === displayUrl(url).toLowerCase()) return [];
		const list = KNOWN.filter((k) => k.includes(q)).slice(0, 5).map((k) => normalizeInput(k));
		const typed = normalizeInput(q);
		if (!list.includes(typed)) list.unshift(typed);
		return list.slice(0, 6);
	});

	$effect(() => {
		if (!focused) text = displayUrl(url);
	});

	$effect(() => {
		if (browser.omniboxFocus) setTimeout(() => input?.focus(), 0);
	});

	function go(href: string) {
		browser.navigate(href);
		input.blur();
	}

	function onkeydown(e: KeyboardEvent) {
		if (e.key === 'Enter') go(suggestions[selected] ?? text);
		else if (e.key === 'Escape') input.blur();
		else if (e.key === 'ArrowDown') {
			selected = Math.min(selected + 1, suggestions.length - 1);
			e.preventDefault();
		} else if (e.key === 'ArrowUp') {
			selected = Math.max(selected - 1, 0);
			e.preventDefault();
		} else selected = 0;
	}

	function label(href: string) {
		if (href.startsWith('mast://search')) return `Search “${new URL(href).searchParams.get('q')}”`;
		return displayUrl(href);
	}
</script>

<div class="omni" class:focused class:sealed>
	<button
		class="status"
		class:ember={sealed}
		aria-label="Site status"
		onclick={() => (statusOpen = !statusOpen)}
	>
		{#if internal}
			<Logo size={16} />
		{:else if sealed}
			<Lock size={15} strokeWidth={2.4} />
		{:else if verdict.kind === 'friction'}
			<Wind size={15} />
		{:else if verdict.kind === 'allow' && verdict.budget}
			<Hourglass size={15} />
		{:else if verdict.kind === 'allow' && verdict.clean.length}
			<Sparkles size={15} />
		{:else}
			<ShieldCheck size={15} />
		{/if}
	</button>

	<input
		bind:this={input}
		bind:value={text}
		spellcheck="false"
		placeholder="Search or type a URL"
		onfocus={() => {
			focused = true;
			text = url === 'mast://newtab' ? '' : url;
			setTimeout(() => input.select(), 0);
		}}
		onblur={() => setTimeout(() => (focused = false), 120)}
		{onkeydown}
	/>

	{#if !focused}
		{#if verdict.kind === 'allow' && verdict.budget}
			<span class="pill warn tabular" title="Today's budget left">
				<Hourglass size={12} />
				{formatDuration(verdict.budget.remainingMs)} left
			</span>
		{/if}
		{#if verdict.kind === 'allow' && verdict.clean.length}
			<span class="pill good" title="Distractions hidden by a Clean pact"><Sparkles size={12} /> Cleaned</span>
		{/if}
		{#if sealed && verdict.kind !== 'allow' && verdict.kind !== 'friction'}
			<span class="pill ember tabular"><Lock size={12} /> Sealed until {formatWhen(verdict.until, clock.now)}</span>
		{/if}
	{/if}

	{#if suggestions.length}
		<div class="suggest">
			{#each suggestions as s, i}
				{@const v = pacts.verdict(s)}
				<button class="row" class:sel={i === selected} onmousedown={() => go(s)}>
					{#if s.startsWith('mast://search')}
						<Search size={15} />
					{:else if isInternal(s)}
						<Logo size={15} />
					{:else}
						<Favicon href={s} size={15} />
					{/if}
					<span class="text">{label(s)}</span>
					{#if v.kind === 'block' || v.kind === 'focus-block' || v.kind === 'budget-out'}
						<span class="pill ember small"><Lock size={11} /> sealed until {formatWhen(v.until, clock.now)}</span>
					{:else if v.kind === 'friction'}
						<span class="pill small"><Wind size={11} /> {v.waitSeconds}s wait</span>
					{/if}
				</button>
			{/each}
		</div>
	{/if}

	{#if statusOpen}
		<SiteStatus href={url} onclose={() => (statusOpen = false)} />
	{/if}
</div>

<style>
	.omni {
		position: relative;
		display: flex;
		align-items: center;
		gap: 6px;
		flex: 1;
		height: 34px;
		padding: 0 6px 0 4px;
		border-radius: 999px;
		background: var(--chrome-2);
		border: 1px solid transparent;
		transition:
			background 0.15s,
			border-color 0.15s;
	}
	.omni:hover {
		background: var(--raised);
	}
	.omni.focused {
		background: var(--chrome);
		border-color: var(--ember);
		box-shadow: 0 0 0 3px var(--ember-soft);
		border-radius: 17px;
	}
	.status {
		display: grid;
		place-items: center;
		width: 28px;
		height: 26px;
		border: 0;
		border-radius: 999px;
		background: none;
		color: var(--muted);
		cursor: pointer;
		flex: none;
	}
	.status:hover {
		background: var(--raised-2);
	}
	.status.ember {
		color: var(--ember);
		background: var(--ember-soft);
	}
	input {
		flex: 1;
		min-width: 0;
		height: 100%;
		border: 0;
		outline: 0;
		background: none;
		color: var(--text);
		font-size: 13.5px;
	}
	input::placeholder {
		color: var(--faint);
	}
	.pill {
		display: inline-flex;
		align-items: center;
		gap: 5px;
		height: 24px;
		padding: 0 10px;
		border-radius: 999px;
		font-size: 12px;
		font-weight: 600;
		white-space: nowrap;
		background: var(--raised-2);
		color: var(--muted);
	}
	.pill.small {
		height: 20px;
		padding: 0 8px;
		font-size: 11px;
	}
	.pill.warn {
		background: var(--warn-soft);
		color: var(--warn);
	}
	.pill.good {
		background: var(--good-soft);
		color: var(--good);
	}
	.pill.ember {
		background: var(--ember-soft);
		color: var(--ember-2);
	}
	.suggest {
		position: absolute;
		top: calc(100% + 6px);
		left: -1px;
		right: -1px;
		padding: 6px;
		border-radius: 14px;
		background: var(--chrome);
		border: 1px solid var(--line-2);
		box-shadow: 0 18px 50px rgb(0 0 0 / 0.5);
		z-index: 50;
	}
	.row {
		display: flex;
		align-items: center;
		gap: 12px;
		width: 100%;
		height: 38px;
		padding: 0 12px;
		border: 0;
		border-radius: 9px;
		background: none;
		color: var(--text);
		text-align: left;
		cursor: pointer;
	}
	.row :global(svg) {
		color: var(--muted);
		flex: none;
	}
	.row.sel,
	.row:hover {
		background: var(--raised);
	}
	.text {
		flex: 1;
		overflow: hidden;
		text-overflow: ellipsis;
		white-space: nowrap;
	}
</style>
