<script lang="ts">
	import { FlaskConical, X, FastForward, RotateCcw, Trash2, DoorOpen } from '@lucide/svelte';
	import { browser } from '#lib/mast/browser.svelte.ts';
	import { blocks } from '#lib/mast/blocks.svelte.ts';
	import { clock } from '#lib/mast/clock.svelte.ts';
	import { DAY, HOUR, MINUTE } from '#lib/mast/engine.ts';
	import { ESCAPES, attemptEscape } from '#lib/mast/escapes.ts';

	let open = $state(false);
	const when = $derived(
		new Date(clock.now).toLocaleString([], { weekday: 'short', month: 'short', day: 'numeric', hour: '2-digit', minute: '2-digit', second: '2-digit' })
	);
</script>

<div class="demo">
	{#if open}
		<div class="panel">
			<header>
				<FlaskConical size={15} />
				<strong>Demo controls</strong>
				<span class="chip">not in the product</span>
				<button class="x" aria-label="Close" onclick={() => (open = false)}><X size={15} /></button>
			</header>

			<section>
				<h4>Time machine</h4>
				<div class="clock tabular" class:travel={clock.travelled}>{when}</div>
				<div class="grid">
					<button onclick={() => clock.advance(10 * MINUTE)}><FastForward size={13} /> +10 min</button>
					<button onclick={() => clock.advance(HOUR)}><FastForward size={13} /> +1 hour</button>
					<button onclick={() => clock.advance(DAY)}><FastForward size={13} /> +1 day</button>
					<button disabled={!clock.travelled} onclick={() => clock.reset()}><RotateCcw size={13} /> Real time</button>
				</div>
			</section>

			<section>
				<div class="grid">
					<button
						onclick={() => {
							blocks.reset();
							clock.reset();
							browser.resetTabs();
							browser.toast('Everything reset.');
						}}><Trash2 size={13} /> Reset demo</button
					>
				</div>
			</section>

			<section>
				<h4>Try to escape</h4>
				<div class="list">
					{#each ESCAPES as e}
						<button onclick={() => attemptEscape(e.kind)}><DoorOpen size={13} /> {e.label}</button>
					{/each}
				</div>
			</section>
		</div>
	{/if}
	<button class="fab" class:on={open} onclick={() => (open = !open)}>
		<FlaskConical size={15} /> Demo
		{#if clock.travelled}<span class="dot"></span>{/if}
	</button>
</div>

<style>
	.demo {
		position: fixed;
		right: 16px;
		bottom: 16px;
		z-index: 80;
		display: grid;
		gap: 10px;
		justify-items: end;
	}
	.fab {
		position: relative;
		display: inline-flex;
		align-items: center;
		gap: 8px;
		height: 34px;
		padding: 0 14px;
		border: 1px dashed rgb(255 255 255 / 0.25);
		border-radius: var(--r);
		background: rgb(20 21 24 / 0.9);
		backdrop-filter: blur(8px);
		color: var(--muted);
		font-size: 12.5px;
		font-weight: 600;
		cursor: pointer;
		box-shadow: 0 8px 24px rgb(0 0 0 / 0.4);
	}
	.fab:hover,
	.fab.on {
		color: var(--text);
		border-style: solid;
	}
	.dot {
		width: 7px;
		height: 7px;
		border-radius: 1px;
		background: var(--focus);
	}
	.panel {
		width: 320px;
		max-height: calc(100vh - 140px);
		overflow-y: auto;
		padding: 14px;
		border-radius: var(--r-2);
		background: rgb(20 21 24 / 0.97);
		border: 1px dashed rgb(255 255 255 / 0.2);
		box-shadow: 0 20px 60px rgb(0 0 0 / 0.6);
		backdrop-filter: blur(10px);
		animation: up 0.18s ease-out;
	}
	@keyframes up {
		from {
			transform: translateY(8px);
			opacity: 0;
		}
	}
	header {
		display: flex;
		align-items: center;
		gap: 8px;
	}
	header :global(svg) {
		color: var(--muted);
	}
	header .chip {
		height: 20px;
		font-size: 10.5px;
	}
	.x {
		margin-left: auto;
		display: grid;
		place-items: center;
		width: 26px;
		height: 26px;
		border: 0;
		border-radius: 2px;
		background: none;
		cursor: pointer;
	}
	.x:hover {
		background: var(--raised);
	}
	section {
		margin-top: 14px;
	}
	h4 {
		margin: 0 0 8px;
		font-size: 11px;
		font-weight: 650;
		letter-spacing: 0.08em;
		text-transform: uppercase;
		color: var(--faint);
	}
	.clock {
		margin-bottom: 8px;
		padding: 8px 10px;
		border-radius: 2px;
		background: var(--chrome-2);
		font-family: var(--mono);
		font-size: 12.5px;
	}
	.clock.travel {
		color: var(--focus);
		box-shadow: inset 0 0 0 1px rgb(124 140 255 / 0.3);
	}
	.grid {
		display: grid;
		grid-template-columns: 1fr 1fr;
		gap: 6px;
	}
	.list {
		display: grid;
		gap: 4px;
	}
	.grid button,
	.list button {
		display: flex;
		align-items: center;
		gap: 8px;
		height: 32px;
		padding: 0 10px;
		border: 1px solid var(--line);
		border-radius: 2px;
		background: var(--chrome-2);
		font-size: 12.5px;
		cursor: pointer;
		text-align: left;
	}
	.grid button:hover:not(:disabled),
	.list button:hover {
		background: var(--raised);
	}
	.grid button:disabled {
		opacity: 0.4;
		cursor: default;
	}
	button :global(svg) {
		color: var(--muted);
		flex: none;
	}
</style>
