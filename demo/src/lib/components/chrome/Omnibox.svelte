<script lang="ts">
	import { Lock, ShieldCheck, Search } from '@lucide/svelte';
	import { browser } from '#lib/mast/browser.svelte.ts';
	import { blocks } from '#lib/mast/blocks.svelte.ts';
	import { clock } from '#lib/mast/clock.svelte.ts';
	import { untilText } from '#lib/mast/engine.ts';
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
	const internal = $derived(isInternal(url));
	const sealed = $derived(blocks.blocks(url));
	/** At rest the address reads like a label: the site bright, the path quiet. */
	const shown = $derived(displayUrl(url));
	const cut = $derived(shown.indexOf('/'));
	const site = $derived(cut < 0 ? shown : shown.slice(0, cut));
	const path = $derived(cut < 0 ? '' : shown.slice(cut));

	const KNOWN = [
		'youtube.com',
		'x.com',
		'reddit.com',
		'news.ycombinator.com',
		'github.com',
		'github.com/mast-browser/mast',
		'en.wikipedia.org/wiki/Ulysses_pact',
		'mast://blocklist',
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
			<Logo size={15} />
		{:else if sealed}
			<Lock size={14} strokeWidth={2} />
		{:else}
			<ShieldCheck size={14} strokeWidth={1.75} />
		{/if}
	</button>

	{#if !focused && shown}
		<span class="display" aria-hidden="true"><span class="site" class:sealed>{site}</span>{path}</span>
	{/if}

	<input
		bind:this={input}
		bind:value={text}
		class:veiled={!focused && !!shown}
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

	{#if suggestions.length}
		<div class="suggest">
			{#each suggestions as s, i}
				<button class="row" class:sel={i === selected} onmousedown={() => go(s)}>
					{#if s.startsWith('mast://search')}
						<Search size={15} />
					{:else if isInternal(s)}
						<Logo size={15} />
					{:else}
						<Favicon href={s} size={15} />
					{/if}
					<span class="text">{label(s)}</span>
					{#if blocks.forHref(s)}
						<span class="pill ember"><Lock size={10} /> {untilText(blocks.forHref(s)!.until, clock.now)}</span>
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
	/* A recessed display window: darker than the toolbar, square, no glow. */
	.omni {
		position: relative;
		display: flex;
		align-items: center;
		gap: 4px;
		flex: 1;
		height: 30px;
		padding: 0 6px 0 3px;
		border-radius: var(--r);
		background: var(--bg);
		box-shadow: inset 0 1px 0 rgb(0 0 0 / 0.35);
		transition: box-shadow 0.15s;
	}
	.omni:hover {
		box-shadow:
			inset 0 1px 0 rgb(0 0 0 / 0.35),
			inset 0 0 0 1px var(--line);
	}
	.omni.focused {
		box-shadow: inset 0 0 0 1px var(--line-3);
	}
	.status {
		display: grid;
		place-items: center;
		width: 26px;
		height: 24px;
		border: 0;
		border-radius: 2px;
		background: none;
		color: var(--faint);
		cursor: pointer;
		flex: none;
	}
	.status:hover {
		background: var(--chrome-2);
		color: var(--muted);
	}
	.status.ember {
		color: var(--ember);
	}
	input {
		flex: 1;
		min-width: 0;
		height: 100%;
		border: 0;
		outline: 0;
		background: none;
		color: var(--text);
		font-size: 13px;
	}
	input.veiled {
		color: transparent;
	}
	input::placeholder {
		color: var(--faint);
	}
	.display {
		position: absolute;
		left: 33px;
		right: 8px;
		overflow: hidden;
		white-space: nowrap;
		text-overflow: ellipsis;
		font-size: 13px;
		color: var(--faint);
		pointer-events: none;
	}
	.site {
		color: var(--text);
	}
	.site.sealed {
		color: var(--muted);
		text-decoration: line-through 1.5px var(--ember);
	}
	.pill {
		display: inline-flex;
		align-items: center;
		gap: 5px;
		height: 20px;
		padding: 0 7px;
		border-radius: 2px;
		font-family: var(--mono);
		font-size: 10.5px;
		font-weight: 600;
		white-space: nowrap;
		background: var(--raised-2);
		color: var(--muted);
	}
	.pill.ember {
		background: var(--ember-soft);
		color: var(--ember-2);
	}
	.suggest {
		position: absolute;
		top: calc(100% + 5px);
		left: 0;
		right: 0;
		padding: 4px;
		border-radius: var(--r-2);
		background: var(--chrome);
		border: 1px solid var(--line-2);
		box-shadow: 0 24px 60px rgb(0 0 0 / 0.55);
		z-index: 50;
	}
	.row {
		display: flex;
		align-items: center;
		gap: 12px;
		width: 100%;
		height: 36px;
		padding: 0 10px;
		border: 0;
		border-radius: 2px;
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
	.row.sel {
		box-shadow: inset 2px 0 0 var(--text);
	}
	.text {
		flex: 1;
		overflow: hidden;
		text-overflow: ellipsis;
		white-space: nowrap;
	}
</style>
