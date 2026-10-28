<script lang="ts">
	import { onMount } from 'svelte';
	import { ArrowRight } from '@lucide/svelte';
	import { browser } from '#lib/mast/browser.svelte.ts';
	import { pacts } from '#lib/mast/pacts.svelte.ts';
	import { formatDate } from '#lib/mast/engine.ts';
	import { hostOf } from '#lib/mast/url.ts';
	import type { Verdict } from '#lib/mast/types.ts';
	import Favicon from '../chrome/Favicon.svelte';

	let { verdict, href }: { verdict: Extract<Verdict, { kind: 'friction' }>; href: string } = $props();

	const host = $derived(hostOf(href));
	// The wait restarts whenever this screen is shown — switching tabs doesn't skip it.
	let left = $state(0);
	onMount(() => {
		left = verdict.waitSeconds;
		const t = setInterval(() => {
			if (document.visibilityState === 'visible') left = Math.max(0, left - 1);
		}, 1000);
		return () => clearInterval(t);
	});
	const phase = $derived(left % 8 >= 4 ? 'Breathe in' : 'Breathe out');
</script>

<div class="gate">
	<div class="orb" class:done={left === 0}>
		<div class="ring"></div>
		<span class="n tabular">{left || ''}</span>
	</div>

	{#if left > 0}
		<p class="eyebrow">{phase}</p>
		<h1>Is <span class="host"><Favicon {href} size={20} /> {host}</span> what you came here for?</h1>
	{:else}
		<p class="eyebrow">Your call</p>
		<h1>You waited. Still want it?</h1>
	{/if}

	{#if verdict.pact.reason}
		<blockquote>“{verdict.pact.reason}” <cite>— you, {formatDate(verdict.pact.sealedAt)}</cite></blockquote>
	{/if}

	<div class="actions">
		<button class="btn" onclick={() => browser.leave()}>Never mind</button>
		<button
			class="btn"
			class:primary={left === 0}
			disabled={left > 0}
			onclick={() => pacts.passFriction(verdict.pact, href)}
		>
			{left > 0 ? `Continue in ${left}s` : `Continue to ${host}`}
			<ArrowRight size={16} />
		</button>
	</div>
</div>

<style>
	.gate {
		display: flex;
		flex-direction: column;
		align-items: center;
		justify-content: center;
		min-height: 100%;
		padding: 48px 24px;
		text-align: center;
		background: radial-gradient(60% 60% at 50% 30%, rgb(76 201 217 / 0.12), transparent 70%), var(--bg);
	}
	.orb {
		position: relative;
		display: grid;
		place-items: center;
		width: 180px;
		height: 180px;
		margin-bottom: 36px;
	}
	.ring {
		position: absolute;
		inset: 0;
		border-radius: 50%;
		background: radial-gradient(circle, rgb(76 201 217 / 0.35), rgb(76 201 217 / 0.06) 70%);
		box-shadow: 0 0 60px rgb(76 201 217 / 0.25);
		animation: breathe 8s ease-in-out infinite;
	}
	.orb.done .ring {
		animation: none;
		transform: scale(0.7);
		opacity: 0.6;
		transition: all 0.6s;
	}
	@keyframes breathe {
		0%,
		100% {
			transform: scale(0.55);
		}
		50% {
			transform: scale(1);
		}
	}
	.n {
		position: relative;
		font-size: 34px;
		font-weight: 700;
		font-family: var(--mono);
	}
	.eyebrow {
		margin: 0 0 10px;
		color: #4cc9d9;
	}
	h1 {
		max-width: 620px;
		margin: 0;
		font-size: 30px;
		line-height: 1.2;
		letter-spacing: -0.02em;
	}
	.host {
		display: inline-flex;
		align-items: center;
		gap: 8px;
		vertical-align: top;
	}
	blockquote {
		max-width: 520px;
		margin: 22px 0 0;
		font-family: 'Iowan Old Style', 'Palatino Linotype', Georgia, serif;
		font-style: italic;
		font-size: 19px;
		color: var(--text);
	}
	cite {
		display: block;
		margin-top: 6px;
		font: 13px var(--font);
		color: var(--muted);
	}
	.actions {
		display: flex;
		gap: 8px;
		margin-top: 32px;
	}
</style>
