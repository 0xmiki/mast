<script lang="ts">
	import { Check, Minus, ChevronDown, Lock, Plus, X } from '@lucide/svelte';
	import { browser } from '#lib/mast/browser.svelte.ts';
	import { blocks, type Block, type Category } from '#lib/mast/blocks.svelte.ts';
	import { clock } from '#lib/mast/clock.svelte.ts';
	import { untilText } from '#lib/mast/engine.ts';
	import { cleanDomain } from '#lib/mast/url.ts';
	import Favicon from './chrome/Favicon.svelte';
	import Fuse from './Fuse.svelte';
	import BlockAction from './BlockAction.svelte';

	let selected = $state<string[]>([]);
	let open = $state<string | null>(null);
	let adding = $state('');

	/** Sites that can still be blocked (or blocked for longer). Forever is final. */
	const selectable = (sites: string[]) => sites.filter((d) => blocks.of(d)?.until !== null);

	function summary(c: Category) {
		const blocked = c.sites.map((d) => blocks.of(d)).filter((b): b is Block => !!b);
		if (!c.sites.length) return { kind: 'empty' as const };
		if (!blocked.length) return { kind: 'open' as const };
		if (blocked.length < c.sites.length) return { kind: 'some' as const, n: blocked.length };
		// All blocked: show the one that ends first.
		const first = blocked.reduce((a, b) => (a.until === null ? b : b.until === null ? a : b.until < a.until ? b : a));
		return { kind: 'all' as const, block: first };
	}

	function toggle(domains: string[]) {
		const all = domains.every((d) => selected.includes(d));
		selected = all ? selected.filter((d) => !domains.includes(d)) : [...new Set([...selected, ...domains])];
	}

	function add(e: SubmitEvent) {
		e.preventDefault();
		const d = cleanDomain(adding);
		if (!d) return;
		if (!blocks.addSite(d)) browser.toast(`${d} is already on the list.`);
		else selected = [...selected, d];
		adding = '';
	}

	const everything = $derived(selectable(blocks.allSites));
	const what = $derived(!selected.length ? '' : selected.length === 1 ? selected[0] : `${selected.length} sites`);
</script>

<div class="panel">
	<div class="head">
		<span class="eyebrow">Distractions</span>
		<button class="link" disabled={!everything.length} onclick={() => toggle(everything)}>
			{everything.length && everything.every((d) => selected.includes(d)) ? 'Select none' : 'Select all'}
		</button>
	</div>

	<ul class="cats">
		{#each blocks.categories as c (c.id)}
			{@const s = summary(c)}
			{@const free = selectable(c.sites)}
			{@const picked = free.filter((d) => selected.includes(d)).length}
			{@const expanded = open === c.id}
			<li class:expanded>
				<div class="row">
					{#if c.sites.length && !free.length}
						<span class="box sealed" title="Blocked forever"><Lock size={11} strokeWidth={2.4} /></span>
					{:else}
						<button
							class="box"
							class:on={free.length > 0 && picked === free.length}
							class:part={picked > 0 && picked < free.length}
							aria-label="Select {c.name}"
							disabled={!free.length}
							onclick={() => toggle(free)}
						>
							{#if picked > 0 && picked === free.length}<Check size={12} strokeWidth={3} />{:else if picked}<Minus size={12} strokeWidth={3} />{/if}
						</button>
					{/if}
					<button class="name" onclick={() => (open = expanded ? null : c.id)}>
						<span>{c.name}</span>
						<span class="favs">
							{#each c.sites.slice(0, 5) as d}<Favicon href="https://{d}" size={14} />{/each}
							{#if c.sites.length > 5}<span class="more mono">+{c.sites.length - 5}</span>{/if}
							{#if !c.sites.length}<span class="more">add your own</span>{/if}
						</span>
						<span class="status">
							{#if s.kind === 'all'}
								<Fuse block={s.block} width={40} />
								<span class="mono ember">{untilText(s.block.until, clock.now)}</span>
							{:else if s.kind === 'some'}
								<span class="mono">{s.n} of {c.sites.length} blocked</span>
							{:else if s.kind === 'open'}
								<span class="mono faint">{c.sites.length} {c.sites.length === 1 ? 'site' : 'sites'}</span>
							{/if}
						</span>
						<ChevronDown size={14} class="chev" />
					</button>
				</div>

				{#if expanded}
					<ul class="sites">
						{#each c.sites as d (d)}
							{@const b = blocks.of(d)}
							<li>
								{#if b?.until === null}
									<span class="box sealed small"><Lock size={9} strokeWidth={2.6} /></span>
								{:else}
									<button class="box small" class:on={selected.includes(d)} aria-label="Select {d}" onclick={() => toggle([d])}>
										{#if selected.includes(d)}<Check size={10} strokeWidth={3} />{/if}
									</button>
								{/if}
								<Favicon href="https://{d}" size={14} />
								<span class="domain" class:struck={!!b}>{d}</span>
								{#if blocks.stoppedFor(d)}<span class="mono faint stopped">stopped {blocks.stoppedFor(d)}×</span>{/if}
								<span class="status">
									{#if b}
										<Fuse block={b} width={28} />
										<span class="mono ember">{untilText(b.until, clock.now)}</span>
									{/if}
								</span>
								{#if c.custom && !b}
									<button class="rm" aria-label="Remove {d}" onclick={() => blocks.removeSite(d)}><X size={12} /></button>
								{/if}
							</li>
						{/each}
						{#if c.custom}
							<li class="add">
								<form onsubmit={add}>
									<Plus size={13} />
									<input bind:value={adding} placeholder="Add a site, e.g. espn.com" spellcheck="false" />
								</form>
							</li>
						{/if}
					</ul>
				{/if}
			</li>
		{/each}
	</ul>

	<div class="bar">
		<span class="picked">
			{#if selected.length}<span class="mono">{selected.length}</span> selected
				<button class="link" onclick={() => (selected = [])}>clear</button>
			{:else}Select what to block{/if}
		</span>
		<BlockAction domains={selected} {what} onblocked={() => (selected = [])} />
	</div>
</div>

<style>
	.panel {
		width: min(820px, 100%);
	}
	.head {
		display: flex;
		align-items: baseline;
		justify-content: space-between;
		padding: 0 2px 10px;
	}
	.link {
		border: 0;
		padding: 0;
		background: none;
		color: var(--muted);
		font-size: 12.5px;
		text-decoration: underline;
		text-decoration-color: var(--line-3);
		text-underline-offset: 3px;
		cursor: pointer;
	}
	.link:hover {
		color: var(--text);
	}
	.link:disabled {
		opacity: 0.4;
		cursor: default;
	}

	/* A ruled ledger: hairlines between rows, nothing rounded. */
	.cats,
	.sites {
		margin: 0;
		padding: 0;
		list-style: none;
	}
	.cats {
		border-top: 1px solid var(--line-2);
	}
	.cats > li {
		border-bottom: 1px solid var(--line);
	}
	.row {
		display: flex;
		align-items: center;
		gap: 4px;
		padding-left: 4px;
	}
	.box {
		display: grid;
		place-items: center;
		width: 16px;
		height: 16px;
		margin: 0 10px 0 6px;
		padding: 0;
		border: 1px solid var(--line-3);
		border-radius: 2px;
		background: none;
		color: #1a0b05;
		cursor: pointer;
		flex: none;
	}
	.box.on,
	.box.part {
		border-color: var(--text);
		background: var(--text);
	}
	.box.sealed {
		border-color: transparent;
		color: var(--ember);
		cursor: default;
	}
	.box.small {
		width: 14px;
		height: 14px;
		margin: 0 4px 0 0;
	}
	.name {
		display: flex;
		align-items: center;
		gap: 16px;
		flex: 1;
		min-width: 0;
		height: 50px;
		padding: 0 10px 0 0;
		border: 0;
		background: none;
		text-align: left;
		font-size: 14.5px;
		font-weight: 550;
		cursor: pointer;
	}
	.name > span:first-child {
		width: 150px;
		flex: none;
	}
	.favs {
		display: flex;
		align-items: center;
		gap: 6px;
		flex: 1;
		min-width: 0;
		filter: grayscale(1);
		opacity: 0.55;
		transition:
			filter 0.2s,
			opacity 0.2s;
	}
	.row:hover .favs,
	.expanded .favs {
		filter: none;
		opacity: 0.9;
	}
	.more {
		font-size: 11px;
		font-weight: 400;
		color: var(--faint);
	}
	.status {
		display: flex;
		align-items: center;
		gap: 10px;
		font-size: 11.5px;
		font-weight: 400;
		color: var(--muted);
		white-space: nowrap;
	}
	.ember {
		color: var(--ember-2);
	}
	.faint {
		color: var(--faint);
	}
	.name :global(.chev) {
		color: var(--faint);
		transition: transform 0.15s;
		flex: none;
	}
	.expanded .name :global(.chev) {
		transform: rotate(180deg);
	}
	.row:hover .name :global(.chev) {
		color: var(--text);
	}

	.sites {
		padding: 2px 0 12px 36px;
	}
	.sites li {
		display: flex;
		align-items: center;
		gap: 10px;
		height: 32px;
		padding-right: 34px;
		font-size: 13px;
	}
	.domain {
		color: var(--muted);
	}
	.domain.struck {
		color: var(--faint);
		text-decoration: line-through 1.5px var(--ember);
	}
	.stopped {
		font-size: 10.5px;
	}
	.sites .status {
		margin-left: auto;
	}
	.rm {
		display: grid;
		place-items: center;
		width: 20px;
		height: 20px;
		margin-left: auto;
		border: 0;
		border-radius: 2px;
		background: none;
		color: var(--faint);
		cursor: pointer;
	}
	.rm:hover {
		background: var(--raised);
		color: var(--text);
	}
	.add form {
		display: flex;
		align-items: center;
		gap: 10px;
		width: 100%;
		color: var(--faint);
	}
	.add input {
		flex: 1;
		height: 30px;
		border: 0;
		border-bottom: 1px solid var(--line-2);
		outline: 0;
		background: none;
		color: var(--text);
		font-size: 13px;
	}
	.add input:focus {
		border-bottom-color: var(--line-3);
	}

	/* Stays in reach while the list scrolls. */
	.bar {
		position: sticky;
		bottom: 14px;
		z-index: 3;
		display: flex;
		align-items: center;
		gap: 24px;
		margin-top: 18px;
		box-shadow:
			inset 0 0 0 1px var(--line),
			0 -12px 40px rgb(0 0 0 / 0.5);
		padding: 14px 16px 14px 18px;
		border-radius: var(--r-2);
		background: var(--chrome);
	}
	.picked {
		width: 150px;
		flex: none;
		color: var(--faint);
		font-size: 12.5px;
	}
	.picked .mono {
		color: var(--text);
	}
	.picked .link {
		margin-left: 6px;
	}
	.bar > :global(.act) {
		flex: 1;
	}
</style>
