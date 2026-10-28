<script lang="ts">
	import { Plus, Sparkles, ChevronRight } from '@lucide/svelte';
	import { browser } from '#lib/mast/browser.svelte.ts';
	import { pacts } from '#lib/mast/pacts.svelte.ts';
	import { clock } from '#lib/mast/clock.svelte.ts';
	import { covers, describe, enforcing, isOpen, KIND_LABELS } from '#lib/mast/engine.ts';
	import { hostOf, isInternal } from '#lib/mast/url.ts';
	import { KIND_COLOR } from '#lib/mast/ui.ts';
	import KindIcon from '../KindIcon.svelte';

	let { href, onclose }: { href: string; onclose: () => void } = $props();

	const host = $derived(hostOf(href));
	const related = $derived(
		pacts.pacts.filter(
			(p) => isOpen(p, clock.now) && (p.rule.kind === 'focus' ? !covers(p, host) : covers(p, host))
		)
	);

	function act(fn: () => void) {
		fn();
		onclose();
	}
</script>

<svelte:window onkeydown={(e) => e.key === 'Escape' && onclose()} />
<button class="scrim" aria-label="Close" onclick={onclose}></button>
<div class="pop">
	{#if isInternal(href)}
		<div class="head">
			<strong>Mast page</strong>
			<span>Built into the browser. Pacts can’t be changed from anywhere else.</span>
		</div>
	{:else}
		<div class="head">
			<strong>{host}</strong>
			<span>
				{#if related.length}
					{related.length} pact{related.length === 1 ? '' : 's'} cover{related.length === 1 ? 's' : ''} this site
				{:else}
					No pacts touch this site.
				{/if}
			</span>
		</div>
		{#each related as p (p.id)}
			<a class="pact" href="mast://pacts/{p.id}" onclick={(e) => {
				e.preventDefault();
				act(() => browser.navigate(`mast://pacts/${p.id}`));
			}}>
				<span class="icon" style:color={KIND_COLOR[p.rule.kind]}><KindIcon kind={p.rule.kind} /></span>
				<span class="body">
					<span class="kind">
						{KIND_LABELS[p.rule.kind]}
						{#if enforcing(p, clock.now)}<em>in force</em>{:else}<em class="off">waiting</em>{/if}
					</span>
					<span class="desc">{describe(p)}</span>
				</span>
				<ChevronRight size={15} />
			</a>
		{/each}
		<div class="actions">
			<button class="btn primary small" onclick={() => act(() => browser.newPact({ kind: 'block', sites: [host] }))}>
				<Plus size={14} /> Seal {host}
			</button>
			<button class="btn small" onclick={() => act(() => browser.newPact({ kind: 'clean', sites: [host] }))}>
				<Sparkles size={14} /> Clean it up
			</button>
		</div>
	{/if}
</div>

<style>
	.scrim {
		position: fixed;
		inset: 0;
		z-index: 40;
		border: 0;
		background: none;
		cursor: default;
	}
	.pop {
		position: absolute;
		top: calc(100% + 8px);
		left: 0;
		z-index: 41;
		width: 360px;
		padding: 8px;
		border-radius: 16px;
		background: var(--chrome);
		border: 1px solid var(--line-2);
		box-shadow: 0 18px 50px rgb(0 0 0 / 0.55);
	}
	.head {
		display: grid;
		gap: 2px;
		padding: 10px 12px 12px;
	}
	.head strong {
		font-size: 14px;
	}
	.head span {
		color: var(--muted);
		font-size: 12.5px;
	}
	.pact {
		display: flex;
		align-items: center;
		gap: 12px;
		padding: 10px 12px;
		border-radius: 10px;
		color: var(--text);
		text-decoration: none;
	}
	.pact:hover {
		background: var(--raised);
	}
	.pact > :global(svg) {
		color: var(--faint);
		flex: none;
	}
	.icon {
		display: grid;
		place-items: center;
		width: 30px;
		height: 30px;
		border-radius: 9px;
		background: var(--raised);
		flex: none;
	}
	.body {
		display: grid;
		gap: 2px;
		min-width: 0;
		flex: 1;
	}
	.kind {
		font-weight: 600;
		font-size: 13px;
	}
	.kind em {
		margin-left: 6px;
		font-style: normal;
		font-size: 11px;
		color: var(--ember-2);
	}
	.kind em.off {
		color: var(--faint);
	}
	.desc {
		color: var(--muted);
		font-size: 12px;
	}
	.actions {
		display: flex;
		gap: 8px;
		padding: 10px 12px 6px;
		border-top: 1px solid var(--line);
		margin-top: 6px;
	}
</style>
