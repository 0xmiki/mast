<script lang="ts">
	import { Lock, ShieldCheck, Clock, DoorOpen, Info, Puzzle, Code, Trash2, CircleCheck } from '@lucide/svelte';
	import { browser } from '#lib/mast/browser.svelte.ts';

	const ICONS = { lock: Lock, shield: ShieldCheck, clock: Clock, door: DoorOpen, info: Info, puzzle: Puzzle, code: Code, trash: Trash2 };
	const Icon = $derived(browser.dialog ? ICONS[browser.dialog.icon] : Info);
</script>

<svelte:window onkeydown={(e) => e.key === 'Escape' && (browser.dialog = null)} />

<div class="toasts">
	{#each browser.toasts as t (t.id)}
		<div class="toast {t.tone}">
			{#if t.tone === 'lock'}<Lock size={14} />{:else if t.tone === 'good'}<CircleCheck size={14} />{:else}<Info size={14} />{/if}
			{t.text}
		</div>
	{/each}
</div>

{#if browser.dialog}
	{@const d = browser.dialog}
	<div class="scrim" role="presentation" onclick={() => (browser.dialog = null)}>
		<div class="dialog" role="dialog" aria-modal="true" tabindex="-1" onclick={(e) => e.stopPropagation()} onkeydown={() => {}}>
			<div class="icon" class:door={d.icon === 'door'}><Icon size={22} /></div>
			{#if d.tag}<span class="chip warn">{d.tag}</span>{/if}
			<h2>{d.title}</h2>
			<p>{d.body}</p>
			{#if d.note}<p class="note">{d.note}</p>{/if}
			<button class="btn" onclick={() => (browser.dialog = null)}>Got it</button>
		</div>
	</div>
{/if}

<style>
	.toasts {
		position: fixed;
		bottom: 24px;
		left: 50%;
		transform: translateX(-50%);
		display: grid;
		gap: 8px;
		justify-items: center;
		z-index: 100;
		pointer-events: none;
	}
	.toast {
		display: flex;
		align-items: center;
		gap: 10px;
		padding: 10px 16px;
		border-radius: 12px;
		background: #26282e;
		border: 1px solid var(--line-2);
		box-shadow: 0 12px 30px rgb(0 0 0 / 0.5);
		font-size: 13px;
		animation: up 0.2s ease-out;
	}
	.toast.lock :global(svg) {
		color: var(--ember);
	}
	.toast.good :global(svg) {
		color: var(--good);
	}
	@keyframes up {
		from {
			transform: translateY(8px);
			opacity: 0;
		}
	}
	.scrim {
		position: fixed;
		inset: 0;
		z-index: 90;
		display: grid;
		place-items: center;
		background: rgb(0 0 0 / 0.55);
		backdrop-filter: blur(4px);
		animation: fade 0.15s;
	}
	@keyframes fade {
		from {
			opacity: 0;
		}
	}
	.dialog {
		display: grid;
		justify-items: center;
		gap: 12px;
		width: min(440px, calc(100vw - 32px));
		padding: 30px 28px 24px;
		border-radius: 20px;
		background: var(--chrome);
		border: 1px solid var(--line-2);
		box-shadow: 0 30px 90px rgb(0 0 0 / 0.6);
		text-align: center;
		animation: pop 0.22s cubic-bezier(0.2, 1.3, 0.4, 1);
	}
	@keyframes pop {
		from {
			transform: scale(0.94);
			opacity: 0;
		}
	}
	.icon {
		display: grid;
		place-items: center;
		width: 52px;
		height: 52px;
		border-radius: 50%;
		background: var(--ember-soft);
		color: var(--ember);
	}
	.icon.door {
		background: var(--warn-soft);
		color: var(--warn);
	}
	h2 {
		margin: 4px 0 0;
		font-size: 20px;
		letter-spacing: -0.01em;
	}
	p {
		margin: 0;
		color: var(--muted);
	}
	.note {
		font-size: 12.5px;
		color: var(--faint);
	}
	.btn {
		margin-top: 8px;
		min-width: 120px;
	}
</style>
