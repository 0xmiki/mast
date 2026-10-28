<script lang="ts">
	import { Lock, Crosshair, Hourglass, ArrowLeft } from '@lucide/svelte';
	import { browser } from '#lib/mast/browser.svelte.ts';
	import { pacts } from '#lib/mast/pacts.svelte.ts';
	import { clock } from '#lib/mast/clock.svelte.ts';
	import { formatDate, formatWhen, RELEASE_MS, formatDuration } from '#lib/mast/engine.ts';
	import { hostOf } from '#lib/mast/url.ts';
	import type { Verdict } from '#lib/mast/types.ts';
	import Favicon from '../chrome/Favicon.svelte';

	type Sealed = Extract<Verdict, { kind: 'block' | 'focus-block' | 'budget-out' }>;
	let { verdict, href }: { verdict: Sealed; href: string } = $props();

	const host = $derived(hostOf(href));
	const pact = $derived(verdict.pact);
	const left = $derived(verdict.until - clock.now);
	const attempts = $derived(pacts.attemptsToday(host));
	let confirmRelease = $state(false);

	function countdown(ms: number) {
		if (!Number.isFinite(ms)) return '∞';
		const s = Math.max(0, Math.floor(ms / 1000));
		const d = Math.floor(s / 86400);
		const hh = String(Math.floor((s % 86400) / 3600)).padStart(2, '0');
		const mm = String(Math.floor((s % 3600) / 60)).padStart(2, '0');
		const ss = String(s % 60).padStart(2, '0');
		return d ? `${d}d ${hh}:${mm}:${ss}` : `${hh}:${mm}:${ss}`;
	}

	function ordinal(n: number) {
		const s = ['th', 'st', 'nd', 'rd'];
		const v = n % 100;
		return n + (s[(v - 20) % 10] || s[v] || s[0]);
	}
</script>

<div class="wall" class:focus={verdict.kind === 'focus-block'} class:budget={verdict.kind === 'budget-out'}>
	<div class="inner">
		<div class="seal">
			{#if verdict.kind === 'focus-block'}
				<Crosshair size={30} />
			{:else if verdict.kind === 'budget-out'}
				<Hourglass size={30} />
			{:else}
				<Lock size={30} />
			{/if}
		</div>

		{#if verdict.kind === 'focus-block'}
			<p class="eyebrow">Focus session</p>
			<h1>Not now. You’re focusing.</h1>
		{:else if verdict.kind === 'budget-out'}
			<p class="eyebrow">Daily budget spent</p>
			<h1>That was today’s {pact.rule.kind === 'budget' ? pact.rule.minutesPerDay : ''} minutes.</h1>
		{:else}
			<p class="eyebrow">Sealed by a pact</p>
			<h1><span class="host"><Favicon {href} size={22} /> {host}</span> is sealed.</h1>
		{/if}

		{#if pact.reason}
			<blockquote>
				<p>“{pact.reason}”</p>
				<cite>— you, {formatDate(pact.sealedAt)}</cite>
			</blockquote>
		{/if}

		<div class="clock">
			<span class="label">{verdict.kind === 'focus-block' ? 'Session ends' : 'Opens'} {formatWhen(verdict.until, clock.now)}</span>
			<span class="time tabular">{countdown(left)}</span>
		</div>

		{#if verdict.kind === 'focus-block'}
			<div class="allowed">
				<span>Open during this session:</span>
				{#each pact.sites as s}
					<a href="https://{s}"><Favicon href="https://{s}" size={14} /> {s}</a>
				{/each}
			</div>
		{/if}

		<div class="actions">
			<button class="btn" onclick={() => browser.leave()}><ArrowLeft size={16} /> Go back</button>
			<a class="btn ghost" href="mast://pacts/{pact.id}">See the pact</a>
		</div>

		<p class="ledger">
			{#if attempts > 0}
				Your {ordinal(attempts)} try today. It’s in your ledger.
			{:else}
				No “continue anyway” button. That’s the point.
			{/if}
		</p>

		{#if pact.escape === 'cooldown'}
			<div class="escape">
				{#if pact.releaseRequestedAt}
					Release requested. The pact holds for {formatDuration(pact.releaseRequestedAt + RELEASE_MS - clock.now)} more, then ends as broken.
				{:else if confirmRelease}
					<span>Release starts a 24-hour wait. The pact stays in force meanwhile and is recorded as broken.</span>
					<span class="row">
						<button class="btn small" onclick={() => pacts.requestRelease(pact.id)}>Start the 24h wait</button>
						<button class="btn small ghost" onclick={() => (confirmRelease = false)}>Keep my word</button>
					</span>
				{:else}
					<button class="link" onclick={() => (confirmRelease = true)}>This pact has an escape clause…</button>
				{/if}
			</div>
		{/if}
	</div>
</div>

<style>
	.wall {
		--accent: var(--ember);
		display: grid;
		place-items: center;
		min-height: 100%;
		padding: 48px 24px;
		background:
			radial-gradient(60% 50% at 50% 0%, color-mix(in srgb, var(--accent) 16%, transparent), transparent 70%),
			var(--bg);
		text-align: center;
	}
	.wall.focus {
		--accent: var(--focus);
	}
	.wall.budget {
		--accent: var(--warn);
	}
	.inner {
		display: flex;
		flex-direction: column;
		align-items: center;
		max-width: 560px;
	}
	.seal {
		display: grid;
		place-items: center;
		width: 76px;
		height: 76px;
		margin-bottom: 28px;
		border-radius: 50%;
		color: var(--accent);
		background: color-mix(in srgb, var(--accent) 14%, transparent);
		box-shadow:
			0 0 0 1px color-mix(in srgb, var(--accent) 35%, transparent),
			0 0 0 10px color-mix(in srgb, var(--accent) 6%, transparent);
		animation: breathe 4s ease-in-out infinite;
	}
	@keyframes breathe {
		50% {
			box-shadow:
				0 0 0 1px color-mix(in srgb, var(--accent) 35%, transparent),
				0 0 0 18px color-mix(in srgb, var(--accent) 3%, transparent);
		}
	}
	.eyebrow {
		margin: 0 0 10px;
		color: var(--accent);
	}
	h1 {
		margin: 0;
		font-size: 34px;
		line-height: 1.15;
		font-weight: 700;
		letter-spacing: -0.02em;
	}
	.host {
		display: inline-flex;
		align-items: center;
		gap: 10px;
		vertical-align: top;
	}
	blockquote {
		margin: 28px 0 0;
		padding: 0;
	}
	blockquote p {
		margin: 0;
		font-family: 'Iowan Old Style', 'Palatino Linotype', Georgia, serif;
		font-style: italic;
		font-size: 22px;
		line-height: 1.4;
		color: var(--text);
	}
	cite {
		display: block;
		margin-top: 8px;
		font-style: normal;
		font-size: 13px;
		color: var(--muted);
	}
	.clock {
		display: grid;
		gap: 4px;
		margin-top: 32px;
		padding: 16px 28px;
		border-radius: 16px;
		background: var(--chrome);
		border: 1px solid var(--line);
	}
	.label {
		font-size: 12.5px;
		color: var(--muted);
	}
	.time {
		font-family: var(--mono);
		font-size: 30px;
		font-weight: 600;
		letter-spacing: 0.02em;
	}
	.allowed {
		display: flex;
		flex-wrap: wrap;
		align-items: center;
		justify-content: center;
		gap: 8px;
		margin-top: 20px;
		font-size: 13px;
		color: var(--muted);
	}
	.allowed a {
		display: inline-flex;
		align-items: center;
		gap: 6px;
		padding: 5px 10px;
		border-radius: 999px;
		background: var(--focus-soft);
		color: var(--text);
		text-decoration: none;
	}
	.actions {
		display: flex;
		gap: 8px;
		margin-top: 28px;
	}
	.ledger {
		margin: 18px 0 0;
		font-size: 12.5px;
		color: var(--faint);
	}
	.escape {
		display: grid;
		gap: 10px;
		justify-items: center;
		margin-top: 36px;
		max-width: 420px;
		font-size: 12.5px;
		color: var(--muted);
	}
	.escape .row {
		display: flex;
		gap: 8px;
	}
	.link {
		border: 0;
		background: none;
		color: var(--faint);
		font-size: 12px;
		text-decoration: underline;
		text-underline-offset: 3px;
		cursor: pointer;
	}
</style>
