<script lang="ts">
	import { Lock, ArrowLeft } from '@lucide/svelte';
	import { browser } from '#lib/mast/browser.svelte.ts';
	import { focus } from '#lib/mast/focus.svelte.ts';
	import { clock } from '#lib/mast/clock.svelte.ts';
	import { clockTime, countdown } from '#lib/mast/engine.ts';
	import { hostOf } from '#lib/mast/url.ts';
	import Favicon from '../chrome/Favicon.svelte';

	let { href }: { href: string } = $props();

	const host = $derived(hostOf(href));
	const session = $derived(focus.active!);
	const tries = $derived(focus.attemptsThisSession());
</script>

<div class="wall">
	<div class="seal"><Lock size={30} /></div>
	<p class="eyebrow">Deep work</p>
	<h1><span class="host"><Favicon {href} size={24} /> {host}</span> is blocked.</h1>

	<div class="clock">
		<span class="time tabular">{countdown(session.endsAt - clock.now)}</span>
		<span class="label">Opens at {clockTime(session.endsAt)}</span>
	</div>

	<button class="btn" onclick={() => browser.leave()}><ArrowLeft size={16} /> Back to work</button>

	{#if tries > 1}
		<p class="note">{tries} distractions blocked this session.</p>
	{/if}
</div>

<style>
	.wall {
		display: flex;
		flex-direction: column;
		align-items: center;
		justify-content: center;
		min-height: 100%;
		padding: 48px 24px;
		text-align: center;
		background:
			radial-gradient(60% 50% at 50% 0%, rgb(255 106 61 / 0.14), transparent 70%),
			var(--bg);
	}
	.seal {
		display: grid;
		place-items: center;
		width: 76px;
		height: 76px;
		margin-bottom: 28px;
		border-radius: 50%;
		color: var(--ember);
		background: var(--ember-soft);
		box-shadow:
			0 0 0 1px rgb(255 106 61 / 0.35),
			0 0 0 10px rgb(255 106 61 / 0.06);
		animation: breathe 4s ease-in-out infinite;
	}
	@keyframes breathe {
		50% {
			box-shadow:
				0 0 0 1px rgb(255 106 61 / 0.35),
				0 0 0 18px rgb(255 106 61 / 0.03);
		}
	}
	.eyebrow {
		margin: 0 0 10px;
		color: var(--ember);
	}
	h1 {
		margin: 0;
		font-size: 34px;
		line-height: 1.15;
		letter-spacing: -0.02em;
	}
	.host {
		display: inline-flex;
		align-items: center;
		gap: 10px;
		vertical-align: top;
	}
	.clock {
		display: grid;
		gap: 6px;
		margin: 36px 0 32px;
	}
	.time {
		font-family: var(--mono);
		font-size: 64px;
		font-weight: 600;
		line-height: 1;
		letter-spacing: -0.02em;
	}
	.label {
		font-size: 13px;
		color: var(--muted);
	}
	.note {
		margin: 18px 0 0;
		font-size: 12.5px;
		color: var(--faint);
	}
</style>
