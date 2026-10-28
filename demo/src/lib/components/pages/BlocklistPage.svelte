<script lang="ts">
	import { Plus, X, Lock } from '@lucide/svelte';
	import { browser } from '#lib/mast/browser.svelte.ts';
	import { focus } from '#lib/mast/focus.svelte.ts';
	import { PRESETS } from '#lib/mast/blocklist.ts';
	import { cleanDomain } from '#lib/mast/url.ts';
	import Favicon from '../chrome/Favicon.svelte';

	let input = $state('');
	const domain = $derived(cleanDomain(input));

	function add(e: SubmitEvent) {
		e.preventDefault();
		if (!domain) return;
		if (!focus.addSite(domain)) browser.toast(`${domain} is already on the list.`);
		input = '';
	}
</script>

<div class="page">
	<div class="wrap">
		<h1>Blocklist</h1>
		<p class="sub">Every session blocks all of these. The usual suspects are already here.</p>

		<section class="card">
			<h2>Your additions</h2>
			<form onsubmit={add}>
				<input bind:value={input} placeholder="Add a site, e.g. espn.com" spellcheck="false" />
				<button class="btn primary small" disabled={!domain}><Plus size={14} /> Add</button>
			</form>
			{#if focus.custom.length}
				<div class="chips">
					{#each focus.custom as d}
						<span class="chip-site">
							<Favicon href="https://{d}" size={14} />
							{d}
							{#if focus.active}
								<span class="lock" title="Can’t remove during a session"><Lock size={11} /></span>
							{:else}
								<button aria-label="Remove {d}" onclick={() => focus.removeSite(d)}><X size={12} /></button>
							{/if}
						</span>
					{/each}
				</div>
			{/if}
			{#if focus.active}
				<p class="hint"><Lock size={12} /> A session is running: you can add sites now, but not remove them.</p>
			{/if}
		</section>

		{#each PRESETS as g}
			<section>
				<h2 class="eyebrow">{g.group}</h2>
				<div class="chips">
					{#each g.sites as d}
						<span class="chip-site preset"><Favicon href="https://{d}" size={14} />{d}</span>
					{/each}
				</div>
			</section>
		{/each}
	</div>
</div>

<style>
	.page {
		min-height: 100%;
		padding: 36px 24px 64px;
		background: var(--bg);
	}
	.wrap {
		max-width: 760px;
		margin: 0 auto;
	}
	h1 {
		margin: 0;
		font-size: 28px;
		letter-spacing: -0.02em;
	}
	.sub {
		margin: 6px 0 24px;
		color: var(--muted);
	}
	.card {
		padding: 18px 20px;
		margin-bottom: 28px;
	}
	.card h2 {
		margin: 0 0 12px;
		font-size: 14px;
	}
	section h2.eyebrow {
		margin: 22px 0 10px;
	}
	form {
		display: flex;
		gap: 8px;
	}
	input {
		flex: 1;
		height: 36px;
		padding: 0 12px;
		border-radius: 10px;
		border: 1px solid var(--line-2);
		background: var(--chrome-2);
		color: var(--text);
		outline: 0;
	}
	input:focus {
		border-color: var(--ember);
	}
	form .btn {
		height: 36px;
	}
	.chips {
		display: flex;
		flex-wrap: wrap;
		gap: 6px;
	}
	.card .chips {
		margin-top: 12px;
	}
	.chip-site {
		display: inline-flex;
		align-items: center;
		gap: 8px;
		height: 32px;
		padding: 0 6px 0 10px;
		border-radius: 9px;
		background: var(--raised);
		font-size: 13px;
	}
	.chip-site.preset {
		padding-right: 12px;
		background: var(--chrome);
		border: 1px solid var(--line);
		color: var(--muted);
	}
	.chip-site button,
	.lock {
		display: grid;
		place-items: center;
		width: 20px;
		height: 20px;
		border: 0;
		border-radius: 5px;
		background: none;
		color: var(--muted);
		cursor: pointer;
	}
	.lock {
		color: var(--ember);
		cursor: default;
	}
	.chip-site button:hover {
		background: var(--raised-2);
		color: var(--text);
	}
	.hint {
		display: flex;
		align-items: center;
		gap: 6px;
		margin: 12px 0 0;
		font-size: 12.5px;
		color: var(--ember-2);
	}
</style>
