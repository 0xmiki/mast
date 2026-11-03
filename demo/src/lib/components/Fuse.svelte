<script lang="ts">
	/**
	 * How a block looks: a timed block is a lit fuse burning toward its start;
	 * a forever block is a solid ember line that never burns.
	 */
	import { clock } from '#lib/mast/clock.svelte.ts';
	import type { Block } from '#lib/mast/blocks.svelte.ts';

	let { block, width = 44, thick = 2 }: { block: Block; width?: number | string; thick?: number } = $props();

	const left = $derived(
		block.until === null ? 1 : Math.max(0, Math.min(1, (block.until - clock.now) / (block.until - block.since)))
	);
</script>

<span
	class="fuse"
	class:forever={block.until === null}
	style:width={typeof width === 'number' ? `${width}px` : width}
	style:--thick="{thick}px"
	aria-hidden="true"
>
	<span class="cord" style:width="{left * 100}%"></span>
	{#if block.until !== null}<span class="tip" style:left="{left * 100}%"></span>{/if}
</span>

<style>
	.fuse {
		position: relative;
		display: inline-block;
		height: var(--thick);
		background: var(--ash);
		vertical-align: middle;
		flex: none;
	}
	.cord {
		position: absolute;
		inset: 0 auto 0 0;
		background: linear-gradient(90deg, rgb(255 106 61 / 0.45), var(--ember) 75%, var(--ember-2));
		transition: width 1s linear;
	}
	.forever .cord {
		background: var(--ember);
	}
	.tip {
		position: absolute;
		top: 50%;
		width: calc(var(--thick) + 3px);
		height: calc(var(--thick) + 3px);
		border-radius: 50%;
		transform: translate(-50%, -50%);
		background: #ffd9c4;
		box-shadow:
			0 0 4px 1px var(--ember-2),
			0 0 12px 3px rgb(255 106 61 / 0.45);
		transition: left 1s linear;
		animation: smoulder 2.6s ease-in-out infinite;
	}
	@keyframes smoulder {
		50% {
			opacity: 0.7;
		}
	}
</style>
