<script lang="ts">
	import { ArrowLeft } from '@lucide/svelte';
	import { browser } from '#lib/mast/browser.svelte.ts';
	import { blocks } from '#lib/mast/blocks.svelte.ts';
	import { clock } from '#lib/mast/clock.svelte.ts';
	import { leftText, untilText } from '#lib/mast/engine.ts';
	import { hostOf } from '#lib/mast/url.ts';

	let { href }: { href: string } = $props();

	const host = $derived(hostOf(href));
	const block = $derived(blocks.forHref(href)!);
	const left = $derived(
		block.until === null ? 1 : Math.max(0, Math.min(1, (block.until - clock.now) / (block.until - block.since)))
	);
	const since = $derived(new Date(block.since).toLocaleDateString([], { month: 'long', day: 'numeric', year: 'numeric' }));
	const tries = $derived(blocks.stoppedFor(block.domain));
</script>

<div class="wall">
	<p class="eyebrow">Blocked {untilText(block.until, clock.now)}</p>

	<!-- The site's name, struck through by its own block: a burning fuse, or a solid line for forever. -->
	<h1 class:forever={block.until === null}>
		<span class="host">{host}</span>
		<span class="strike" style:width="{left * 100}%">{#if block.until !== null}<span class="tip"></span>{/if}</span>
	</h1>

	{#if block.until === null}
		<p class="left">You blocked it on {since}.</p>
	{:else}
		<p class="left"><span class="mono">{leftText(block.until, clock.now)}</span></p>
	{/if}

	<button class="btn" onclick={() => browser.leave()}><ArrowLeft size={15} /> Back</button>

	{#if tries > 1}
		<p class="note">Stopped <span class="mono">{tries}</span> times</p>
	{/if}
</div>

<style>
	.wall {
		display: flex;
		flex-direction: column;
		align-items: center;
		justify-content: center;
		min-height: 100%;
		padding: 48px 24px 12vh;
		text-align: center;
		background: var(--bg);
	}
	.eyebrow {
		margin: 0 0 22px;
		color: var(--ember-2);
	}
	h1 {
		position: relative;
		margin: 0;
		padding: 0 6px;
		font-size: clamp(40px, 7vw, 84px);
		font-weight: 600;
		line-height: 1.1;
		letter-spacing: -0.045em;
		color: var(--faint);
	}
	.strike {
		position: absolute;
		left: 0;
		top: 54%;
		height: 3px;
		background: linear-gradient(90deg, rgb(255 106 61 / 0.5), var(--ember) 70%, var(--ember-2));
		transition: width 1s linear;
	}
	.forever .strike {
		background: var(--ember);
	}
	.tip {
		position: absolute;
		right: -3.5px;
		top: 50%;
		width: 7px;
		height: 7px;
		margin-top: -3.5px;
		border-radius: 50%;
		background: #ffd9c4;
		box-shadow:
			0 0 5px 1px var(--ember-2),
			0 0 18px 4px rgb(255 106 61 / 0.5),
			0 0 50px 12px rgb(255 106 61 / 0.15);
		animation: smoulder 2.6s ease-in-out infinite;
	}
	@keyframes smoulder {
		50% {
			opacity: 0.72;
		}
	}
	.left {
		margin: 26px 0 30px;
		color: var(--faint);
		font-size: 13px;
	}
	.left .mono {
		color: var(--text);
		font-size: 15px;
	}
	.note {
		margin: 16px 0 0;
		font-size: 12.5px;
		color: var(--faint);
	}
</style>
