<script lang="ts">
	import { ArrowLeft, Plus, Lock, Undo2, ScrollText, CalendarPlus, ShieldOff } from '@lucide/svelte';
	import { browser } from '#lib/mast/browser.svelte.ts';
	import { pacts } from '#lib/mast/pacts.svelte.ts';
	import { clock } from '#lib/mast/clock.svelte.ts';
	import {
		DAY,
		DAY_NAMES,
		FEATURE_LABELS,
		KIND_LABELS,
		MINUTE,
		RELEASE_MS,
		describe,
		formatDate,
		formatDuration,
		formatWhen,
		progress,
		ruleLine,
		schedulePhrase,
		status
	} from '#lib/mast/engine.ts';
	import { cleanDomain } from '#lib/mast/url.ts';
	import { KIND_COLOR, STATUS_CHIP, SUGGESTED_SITES } from '#lib/mast/ui.ts';
	import type { CleanFeature } from '#lib/mast/types.ts';
	import KindIcon from '../KindIcon.svelte';
	import Favicon from '../chrome/Favicon.svelte';

	let { id }: { id: string } = $props();

	const pact = $derived(pacts.get(id));
	const now = $derived(clock.now);
	const st = $derived(pact ? status(pact, now) : 'kept');
	const open = $derived(st !== 'kept' && st !== 'released');
	const attempts = $derived(pact ? pacts.attempts.filter((a) => a.pactId === pact.id) : []);
	let siteInput = $state('');
	let confirmRelease = $state(false);

	function addSite() {
		if (pact) pacts.addSite(pact, cleanDomain(siteInput));
		siteInput = '';
	}
</script>

{#if !pact}
	<div class="page"><div class="wrap"><h1>No such pact</h1><a href="mast://pacts">Back to pacts</a></div></div>
{:else}
	{@const r = pact.rule}
	<div class="page" style:--k={KIND_COLOR[r.kind]}>
		<div class="wrap">
			<a class="back" href="mast://pacts"><ArrowLeft size={14} /> All pacts</a>

			<div class="hero">
				<span class="icon"><KindIcon kind={r.kind} size={22} /></span>
				<div>
					<div class="meta">
						<span class="eyebrow">{KIND_LABELS[r.kind]} pact</span>
						<span class="chip {STATUS_CHIP[st].tone}">{STATUS_CHIP[st].label}</span>
					</div>
					<h1>{describe(pact)}</h1>
					<p class="line tabular">{ruleLine(pact, now, pacts.usedToday(pact))}</p>
				</div>
			</div>

			{#if pact.reason}
				<blockquote>“{pact.reason}” <cite>— you, {formatDate(pact.sealedAt)}</cite></blockquote>
			{/if}

			{#if st === 'grace'}
				<div class="grace">
					<div>
						<strong class="tabular">Regret window · {formatDuration(pact.graceEndsAt - now)}</strong>
						<span>Made a mistake? Cancel now. After this, it’s binding.</span>
					</div>
					<button
						class="btn small"
						onclick={() => {
							if (pacts.cancel(pact.id)) {
								browser.toast('Pact cancelled inside the regret window.');
								browser.navigate('mast://pacts');
							}
						}}><Undo2 size={14} /> Cancel pact</button
					>
				</div>
			{/if}

			<div class="term card">
				<div class="ends">
					<div><span>Sealed</span><b>{formatDate(pact.sealedAt)}</b></div>
					<div><span>{now < pact.startsAt ? 'Starts' : 'Started'}</span><b>{formatWhen(pact.startsAt, now)}</b></div>
					<div><span>{open ? 'Ends' : 'Ended'}</span><b>{formatWhen(pact.endsAt, now)}</b></div>
					<div><span>Blocked attempts</span><b class="tabular">{attempts.length}</b></div>
				</div>
				<div class="bar"><span style:width="{progress(pact, now) * 100}%"></span></div>
				{#if open && now >= pact.startsAt}
					<p class="left tabular">{formatDuration(pact.endsAt - now)} to go</p>
				{/if}
			</div>

			<div class="cols">
				<section class="card">
					<h2><Lock size={15} /> Terms</h2>
					<ul class="terms">
						{#each pact.sites as s}
							<li>
								<Favicon href="https://{s}" size={15} />
								{s}
								{#if r.kind === 'focus' && open && pact.sites.length > 1}
									<button class="tiny" title="Stricter: remove from allow-list" onclick={() => pacts.removeAllowed(pact, s)}>Remove</button>
								{/if}
							</li>
						{/each}
					</ul>
					<dl>
						{#if r.kind === 'block'}
							<dt>Schedule</dt>
							<dd>{schedulePhrase(r.schedule)}</dd>
						{:else if r.kind === 'budget'}
							<dt>Daily budget</dt>
							<dd>{r.minutesPerDay} minutes · {Math.floor(pacts.usedToday(pact) / MINUTE)} used today</dd>
						{:else if r.kind === 'friction'}
							<dt>Wait</dt>
							<dd>{r.waitSeconds} seconds</dd>
						{:else if r.kind === 'clean'}
							<dt>Hidden</dt>
							<dd>{r.features.map((f) => FEATURE_LABELS[f]).join(', ')}</dd>
						{/if}
						<dt>Escape clause</dt>
						<dd>
							{#if pact.escape === 'none'}None. Holds until the end.{:else if pact.releaseRequestedAt}
								Release requested · {formatDuration(Math.max(0, pact.releaseRequestedAt + RELEASE_MS - now))} left
							{:else}24-hour cool-down{/if}
						</dd>
					</dl>
					<p class="nodelete">There is no edit, pause or delete button. That’s the product.</p>
				</section>

				<section class="card">
					<h2><Plus size={15} /> Tighten</h2>
					{#if !open}
						<p class="muted">This pact is over.</p>
					{:else}
						<p class="muted">Stricter is always allowed. Every change is logged.</p>
						<div class="tighten">
							{#if r.kind !== 'focus'}
								<form onsubmit={(e) => { e.preventDefault(); addSite(); }}>
									<input bind:value={siteInput} placeholder="Add a site" list="mast-sites" spellcheck="false" />
									<datalist id="mast-sites">{#each SUGGESTED_SITES as s}<option value={s}></option>{/each}</datalist>
									<button class="btn small" disabled={!cleanDomain(siteInput)}>Add</button>
								</form>
							{/if}
							<div class="buttons">
								{#if r.kind === 'focus'}
									<button class="btn small" onclick={() => pacts.extend(pact, 15 * MINUTE)}><CalendarPlus size={14} /> +15 min</button>
								{:else}
									<button class="btn small" onclick={() => pacts.extend(pact, DAY)}><CalendarPlus size={14} /> +1 day</button>
									<button class="btn small" onclick={() => pacts.extend(pact, 7 * DAY)}><CalendarPlus size={14} /> +1 week</button>
								{/if}
								{#if r.kind === 'block' && !r.schedule.always}
									{#each [6, 0] as d}
										{#if !r.schedule.days.includes(d)}
											<button class="btn small" onclick={() => pacts.addDay(pact, d)}>Also {DAY_NAMES[d]}</button>
										{/if}
									{/each}
									{#if r.schedule.from < r.schedule.to}
										<button class="btn small" disabled={r.schedule.from < 60} onclick={() => pacts.widen(pact, 'earlier')}>Start 1h earlier</button>
										<button class="btn small" disabled={r.schedule.to > 23 * 60} onclick={() => pacts.widen(pact, 'later')}>End 1h later</button>
									{/if}
									<button class="btn small" onclick={() => pacts.alwaysOn(pact)}>Block at all times</button>
								{/if}
								{#if r.kind === 'budget'}
									<button class="btn small" disabled={r.minutesPerDay <= 0} onclick={() => pacts.lowerBudget(pact, 5)}>−5 min/day</button>
								{/if}
								{#if r.kind === 'friction'}
									<button class="btn small" onclick={() => pacts.lengthenWait(pact, 15)}>+15s wait</button>
								{/if}
								{#if r.kind === 'clean'}
									{#each ['feed', 'shorts', 'recommendations', 'comments'] as f}
										{#if !r.features.includes(f as CleanFeature)}
											<button class="btn small" onclick={() => pacts.addFeature(pact, f as CleanFeature)}>Hide {FEATURE_LABELS[f as CleanFeature]}</button>
										{/if}
									{/each}
								{/if}
								{#if pact.escape === 'cooldown' && !pact.releaseRequestedAt}
									<button class="btn small" onclick={() => pacts.dropEscape(pact)}><ShieldOff size={14} /> Remove escape clause</button>
								{/if}
							</div>
						</div>
					{/if}
				</section>
			</div>

			<section class="card log">
				<h2><ScrollText size={15} /> Amendment log</h2>
				<ol>
					<li><span>{formatWhen(pact.sealedAt, now)}</span>Sealed.</li>
					{#each pact.amendments as a}
						<li><span>{formatWhen(a.at, now)}</span>{a.text}</li>
					{/each}
					{#if st === 'kept'}<li class="kept"><span>{formatWhen(pact.endsAt, now)}</span>Kept. Every day of it.</li>{/if}
				</ol>
			</section>

			{#if pact.escape === 'cooldown' && open && !pact.releaseRequestedAt}
				<div class="release">
					{#if confirmRelease}
						<span>Releasing starts a 24-hour wait. The pact stays in force meanwhile and goes in the ledger as broken.</span>
						<button class="btn small" onclick={() => pacts.requestRelease(pact.id)}>Start the 24h wait</button>
						<button class="btn small ghost" onclick={() => (confirmRelease = false)}>Keep my word</button>
					{:else}
						<button class="link" onclick={() => (confirmRelease = true)}>Request release…</button>
					{/if}
				</div>
			{/if}
		</div>
	</div>
{/if}

<style>
	.page {
		min-height: 100%;
		padding: 28px 24px 64px;
		background: radial-gradient(60% 30% at 50% 0%, color-mix(in srgb, var(--k, var(--ember)) 9%, transparent), transparent), var(--bg);
	}
	.wrap {
		max-width: 860px;
		margin: 0 auto;
	}
	.back {
		display: inline-flex;
		align-items: center;
		gap: 6px;
		color: var(--muted);
		font-size: 13px;
		text-decoration: none;
	}
	.back:hover {
		color: var(--text);
	}
	.hero {
		display: flex;
		gap: 18px;
		margin-top: 22px;
	}
	.icon {
		display: grid;
		place-items: center;
		width: 52px;
		height: 52px;
		border-radius: 16px;
		background: color-mix(in srgb, var(--k) 15%, transparent);
		color: var(--k);
		flex: none;
	}
	.meta {
		display: flex;
		align-items: center;
		gap: 10px;
	}
	h1 {
		margin: 6px 0 0;
		font-size: 26px;
		line-height: 1.25;
		letter-spacing: -0.02em;
	}
	.line {
		margin: 6px 0 0;
		color: var(--muted);
	}
	blockquote {
		margin: 24px 0 0 70px;
		padding-left: 14px;
		border-left: 2px solid var(--k);
		font-family: 'Iowan Old Style', 'Palatino Linotype', Georgia, serif;
		font-style: italic;
		font-size: 19px;
	}
	cite {
		margin-left: 6px;
		font: 13px var(--font);
		color: var(--muted);
	}
	.grace {
		display: flex;
		align-items: center;
		justify-content: space-between;
		gap: 16px;
		margin-top: 24px;
		padding: 14px 16px;
		border-radius: 14px;
		background: var(--warn-soft);
		border: 1px solid rgb(245 184 74 / 0.3);
	}
	.grace div {
		display: grid;
	}
	.grace strong {
		color: var(--warn);
	}
	.grace span {
		color: var(--muted);
		font-size: 13px;
	}
	.card {
		padding: 18px 20px;
		margin-top: 16px;
	}
	.term {
		margin-top: 24px;
	}
	.ends {
		display: grid;
		grid-template-columns: repeat(4, 1fr);
		gap: 12px;
	}
	.ends div {
		display: grid;
	}
	.ends span {
		font-size: 12px;
		color: var(--faint);
	}
	.ends b {
		font-weight: 600;
	}
	.bar {
		height: 6px;
		margin-top: 16px;
		border-radius: 99px;
		background: var(--raised-2);
		overflow: hidden;
	}
	.bar span {
		display: block;
		height: 100%;
		background: var(--k);
		border-radius: inherit;
	}
	.left {
		margin: 8px 0 0;
		font-size: 12.5px;
		color: var(--muted);
	}
	.cols {
		display: grid;
		grid-template-columns: 1fr 1fr;
		gap: 16px;
	}
	.cols .card {
		margin-top: 16px;
	}
	h2 {
		display: flex;
		align-items: center;
		gap: 8px;
		margin: 0 0 12px;
		font-size: 14px;
	}
	h2 :global(svg) {
		color: var(--muted);
	}
	.terms {
		display: grid;
		gap: 6px;
		margin: 0;
		padding: 0;
		list-style: none;
	}
	.terms li {
		display: flex;
		align-items: center;
		gap: 10px;
		font-size: 13.5px;
	}
	.tiny {
		margin-left: auto;
		border: 0;
		background: none;
		color: var(--muted);
		font-size: 12px;
		cursor: pointer;
		text-decoration: underline;
	}
	dl {
		display: grid;
		grid-template-columns: auto 1fr;
		gap: 6px 14px;
		margin: 16px 0 0;
		font-size: 13px;
	}
	dt {
		color: var(--faint);
	}
	dd {
		margin: 0;
	}
	.nodelete {
		margin: 16px 0 0;
		padding-top: 12px;
		border-top: 1px dashed var(--line-2);
		font-size: 12px;
		color: var(--faint);
	}
	.muted {
		margin: 0;
		color: var(--muted);
		font-size: 13px;
	}
	.tighten form {
		display: flex;
		gap: 6px;
		margin-top: 12px;
	}
	.tighten input {
		flex: 1;
		height: 30px;
		padding: 0 10px;
		border-radius: 8px;
		border: 1px solid var(--line-2);
		background: var(--chrome-2);
		color: var(--text);
		outline: 0;
	}
	.buttons {
		display: flex;
		flex-wrap: wrap;
		gap: 6px;
		margin-top: 10px;
	}
	.log ol {
		display: grid;
		gap: 8px;
		margin: 0;
		padding: 0;
		list-style: none;
		font-size: 13px;
	}
	.log li {
		display: flex;
		gap: 16px;
	}
	.log li span {
		width: 150px;
		flex: none;
		color: var(--faint);
	}
	.log li.kept {
		color: var(--good);
	}
	.release {
		display: flex;
		align-items: center;
		justify-content: center;
		gap: 10px;
		margin-top: 28px;
		font-size: 12.5px;
		color: var(--muted);
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
