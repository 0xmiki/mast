<script lang="ts">
	import { ArrowLeft, ArrowRight, X, Plus, Check, Lock } from '@lucide/svelte';
	import { browser } from '#lib/mast/browser.svelte.ts';
	import { pacts, emptyDraft, draftToPact } from '#lib/mast/pacts.svelte.ts';
	import { clock } from '#lib/mast/clock.svelte.ts';
	import {
		clockTime,
		DAY_NAMES,
		describe,
		FEATURE_LABELS,
		formatWhen,
		KIND_LABELS
	} from '#lib/mast/engine.ts';
	import { cleanDomain } from '#lib/mast/url.ts';
	import { KIND_COLOR, SUGGESTED_SITES, WORK_SITES } from '#lib/mast/ui.ts';
	import type { CleanFeature, Draft, PactKind } from '#lib/mast/types.ts';
	import KindIcon from '../KindIcon.svelte';
	import Favicon from '../chrome/Favicon.svelte';
	import Logo from '../Logo.svelte';

	const seed = browser.draftSeed;
	browser.draftSeed = null;

	let draft = $state<Draft>({ ...emptyDraft(), ...seed });
	if (seed?.kind === 'focus' && !seed.sites) draft.sites = ['github.com', 'developer.mozilla.org'];
	let step = $state(seed?.kind ? 1 : 0);
	let siteInput = $state('');
	let phrase = $state('');

	const STEPS = ['Kind', 'Rules', 'Your words', 'Seal'];
	const KINDS: { kind: PactKind; text: string; example: string }[] = [
		{ kind: 'block', text: 'Sites won’t open during set hours.', example: 'Social media, weekdays 9–5' },
		{ kind: 'budget', text: 'A daily allowance of minutes, then sealed.', example: '20 min/day of feeds' },
		{ kind: 'focus', text: 'For one session, only the sites you choose open.', example: '50 min with GitHub + docs' },
		{ kind: 'friction', text: 'A forced pause before a site opens.', example: '30s breath before YouTube' },
		{ kind: 'clean', text: 'Keep the site, lose the hooks.', example: 'YouTube without Shorts' }
	];
	const FEATURES: CleanFeature[] = ['feed', 'shorts', 'recommendations', 'comments'];
	const PROMPTS = [
		'I want my mornings back.',
		'I’m building something that matters more than the feed.',
		'Future me will thank me for this.'
	];
	const PHRASE = 'I give my word';

	const preview = $derived(draftToPact(draft, clock.now));
	const sealingOk = $derived(phrase.trim().toLowerCase() === PHRASE.toLowerCase());
	const rulesOk = $derived(
		draft.sites.length > 0 &&
			(draft.kind !== 'block' || draft.schedule.always || draft.schedule.days.length > 0) &&
			(draft.kind !== 'clean' || draft.features.length > 0)
	);
	const suggestions = $derived(draft.kind === 'focus' ? WORK_SITES : SUGGESTED_SITES);

	function pickKind(k: PactKind) {
		if (k !== draft.kind) {
			draft.kind = k;
			draft.sites = k === 'focus' ? ['github.com', 'developer.mozilla.org'] : draft.sites.filter((s) => !WORK_SITES.includes(s));
		}
		step = 1;
	}

	function addSite(raw: string) {
		const d = cleanDomain(raw);
		if (d && !draft.sites.includes(d)) draft.sites.push(d);
		siteInput = '';
	}

	function toggle<T>(list: T[], item: T) {
		const i = list.indexOf(item);
		if (i >= 0) list.splice(i, 1);
		else list.push(item);
	}

	function timeInput(minutes: number) {
		return clockTime(minutes);
	}
	function parseTime(v: string) {
		const [h, m] = v.split(':').map(Number);
		return h * 60 + (m || 0);
	}

	// ---- press and hold to seal
	let hold = $state(0);
	let holding: number | undefined;
	let sealed = $state(false);
	const HOLD_MS = 1600;

	function startHold() {
		if (!sealingOk || sealed) return;
		const t0 = performance.now();
		const tick = () => {
			hold = Math.min(1, (performance.now() - t0) / HOLD_MS);
			if (hold >= 1) return seal();
			holding = requestAnimationFrame(tick);
		};
		holding = requestAnimationFrame(tick);
	}
	function stopHold() {
		if (holding) cancelAnimationFrame(holding);
		holding = undefined;
		if (!sealed) hold = 0;
	}
	function seal() {
		sealed = true;
		const p = pacts.seal(draft);
		setTimeout(() => browser.navigate(`mast://pacts/${p.id}`), 1400);
	}
</script>

<div class="wizard">
	<header>
		<button class="btn ghost small" onclick={() => (step > 0 ? step-- : browser.leave())}>
			<ArrowLeft size={15} /> {step > 0 ? 'Back' : 'Cancel'}
		</button>
		<ol>
			{#each STEPS as s, i}
				<li class:done={i < step} class:now={i === step}>
					<span>{#if i < step}<Check size={12} strokeWidth={3} />{:else}{i + 1}{/if}</span>{s}
				</li>
			{/each}
		</ol>
		<span style="width: 90px"></span>
	</header>

	{#if step === 0}
		<section>
			<h1>What kind of promise?</h1>
			<p class="lede">Every pact can be made stricter later. None can be made looser.</p>
			<div class="kinds">
				{#each KINDS as k}
					<button class="kind" class:on={draft.kind === k.kind} style:--k={KIND_COLOR[k.kind]} onclick={() => pickKind(k.kind)}>
						<span class="icon"><KindIcon kind={k.kind} size={20} /></span>
						<strong>{KIND_LABELS[k.kind]}</strong>
						<span>{k.text}</span>
						<em>{k.example}</em>
					</button>
				{/each}
			</div>
		</section>
	{:else if step === 1}
		<section>
			<h1>
				<span class="badge" style:--k={KIND_COLOR[draft.kind]}><KindIcon kind={draft.kind} size={18} /></span>
				{KIND_LABELS[draft.kind]} pact
			</h1>

			<div class="field">
				<span class="label">{draft.kind === 'focus' ? 'Sites allowed during the session' : 'Sites'}</span>
				<div class="sites">
					{#each draft.sites as s}
						<span class="site"><Favicon href="https://{s}" size={14} />{s}
							<button aria-label="Remove {s}" onclick={() => toggle(draft.sites, s)}><X size={12} /></button>
						</span>
					{/each}
					<form onsubmit={(e) => { e.preventDefault(); addSite(siteInput); }}>
						<input bind:value={siteInput} placeholder="Add a site, e.g. tiktok.com" spellcheck="false" />
					</form>
				</div>
				<div class="suggest">
					{#each suggestions.filter((s) => !draft.sites.includes(s)) as s}
						<button onclick={() => addSite(s)}><Plus size={12} />{s}</button>
					{/each}
				</div>
			</div>

			{#if draft.kind === 'block'}
				<div class="field">
					<span class="label">When</span>
					<div class="seg">
						<button class:on={!draft.schedule.always} onclick={() => (draft.schedule.always = false)}>On a schedule</button>
						<button class:on={draft.schedule.always} onclick={() => (draft.schedule.always = true)}>Always</button>
					</div>
					{#if !draft.schedule.always}
						<div class="days">
							{#each [1, 2, 3, 4, 5, 6, 0] as d}
								<button class:on={draft.schedule.days.includes(d)} onclick={() => toggle(draft.schedule.days, d)}>{DAY_NAMES[d]}</button>
							{/each}
						</div>
						<div class="times">
							<label>From <input type="time" value={timeInput(draft.schedule.from)} onchange={(e) => (draft.schedule.from = parseTime(e.currentTarget.value))} /></label>
							<label>to <input type="time" value={timeInput(draft.schedule.to)} onchange={(e) => (draft.schedule.to = parseTime(e.currentTarget.value))} /></label>
						</div>
					{/if}
				</div>
			{:else if draft.kind === 'budget'}
				<div class="field">
					<span class="label">Minutes per day, shared across these sites</span>
					<div class="seg">
						{#each [5, 10, 20, 30, 45, 60] as m}
							<button class:on={draft.minutesPerDay === m} onclick={() => (draft.minutesPerDay = m)}>{m} min</button>
						{/each}
					</div>
				</div>
			{:else if draft.kind === 'focus'}
				<div class="field">
					<span class="label">Session length</span>
					<div class="seg">
						{#each [25, 50, 90, 180] as m}
							<button class:on={draft.sessionMinutes === m} onclick={() => (draft.sessionMinutes = m)}>{m < 60 ? `${m} min` : `${m / 60}h`}</button>
						{/each}
					</div>
				</div>
			{:else if draft.kind === 'friction'}
				<div class="field">
					<span class="label">Wait before opening</span>
					<div class="seg">
						{#each [10, 30, 60, 120] as s}
							<button class:on={draft.waitSeconds === s} onclick={() => (draft.waitSeconds = s)}>{s < 60 ? `${s}s` : `${s / 60} min`}</button>
						{/each}
					</div>
				</div>
			{:else if draft.kind === 'clean'}
				<div class="field">
					<span class="label">Hide</span>
					<div class="seg">
						{#each FEATURES as f}
							<button class:on={draft.features.includes(f)} onclick={() => toggle(draft.features, f)}>{FEATURE_LABELS[f]}</button>
						{/each}
					</div>
				</div>
			{/if}

			{#if draft.kind !== 'focus'}
				<div class="field row">
					<div>
						<span class="label">For</span>
						<div class="seg">
							{#each [1, 7, 30, 90] as d}
								<button class:on={draft.days === d} onclick={() => (draft.days = d)}>{d === 1 ? '1 day' : `${d} days`}</button>
							{/each}
						</div>
					</div>
					<div>
						<span class="label">Starting</span>
						<div class="seg">
							<button class:on={draft.startsAt === 'now'} onclick={() => (draft.startsAt = 'now')}>Now</button>
							<button class:on={draft.startsAt === 'tomorrow'} onclick={() => (draft.startsAt = 'tomorrow')}>Tomorrow</button>
						</div>
					</div>
				</div>
			{/if}

			<div class="field">
				<span class="label">Escape clause</span>
				<div class="escapes">
					<button class:on={draft.escape === 'none'} onclick={() => (draft.escape = 'none')}>
						<strong>None</strong>
						<span>No way out until it ends. The real Ulysses pact.</span>
					</button>
					<button class:on={draft.escape === 'cooldown'} onclick={() => (draft.escape = 'cooldown')}>
						<strong>24-hour cool-down</strong>
						<span>You can ask for release; it takes a day and counts as broken.</span>
					</button>
				</div>
			</div>

			<div class="next">
				<p class="sentence">{describe(preview)}</p>
				<button class="btn primary" disabled={!rulesOk} onclick={() => (step = 2)}>Continue <ArrowRight size={16} /></button>
			</div>
		</section>
	{:else if step === 2}
		<section>
			<h1>Why are you doing this?</h1>
			<p class="lede">
				Write it to the version of you who will hit this wall. Mast shows it back every time you
				try to get through.
			</p>
			<!-- svelte-ignore a11y_autofocus -->
			<textarea bind:value={draft.reason} rows="3" maxlength="160" placeholder="In your own words…" autofocus></textarea>
			<div class="prompts">
				{#each PROMPTS as p}<button onclick={() => (draft.reason = p)}>{p}</button>{/each}
			</div>
			<div class="next">
				<span class="count">{draft.reason.length}/160</span>
				<button class="btn primary" disabled={draft.reason.trim().length < 4} onclick={() => (step = 3)}>
					Review & seal <ArrowRight size={16} />
				</button>
			</div>
		</section>
	{:else}
		<section class="sealing">
			<div class="contract" class:sealed>
				<div class="contract-head">
					<Logo size={22} />
					<span class="eyebrow">Pact · {KIND_LABELS[draft.kind]}</span>
				</div>
				<p class="terms">{describe(preview)}</p>
				<dl>
					<div><dt>Starts</dt><dd>{draft.kind === 'focus' || draft.startsAt === 'now' ? 'Now' : formatWhen(preview.startsAt, clock.now)}</dd></div>
					<div><dt>Ends</dt><dd>{formatWhen(preview.endsAt, clock.now)}</dd></div>
					<div><dt>Escape</dt><dd>{draft.escape === 'none' ? 'None' : '24h cool-down, counts as broken'}</dd></div>
				</dl>
				<blockquote>“{draft.reason}”</blockquote>
				<ul class="fine">
					<li>Can be made stricter at any time. Never looser.</li>
					<li>No pause, no delete, no off switch. Not in settings, incognito or another profile.</li>
					<li>2-minute regret window for mistakes, then it’s binding.</li>
				</ul>
				{#if sealed}
					<div class="stamp"><Lock size={22} strokeWidth={2.6} /><span>Sealed</span></div>
				{/if}
			</div>

			{#if !sealed}
				<label class="phrase">
					Type <b>{PHRASE}</b> to unlock the seal
					<input bind:value={phrase} spellcheck="false" placeholder={PHRASE} />
				</label>
				<button
					class="hold"
					class:ready={sealingOk}
					disabled={!sealingOk}
					style:--p={hold}
					onpointerdown={startHold}
					onpointerup={stopHold}
					onpointerleave={stopHold}
					onkeydown={(e) => (e.key === ' ' || e.key === 'Enter') && !e.repeat && startHold()}
					onkeyup={stopHold}
				>
					<span class="fill"></span>
					<span class="text"><Lock size={16} /> {hold > 0 ? 'Keep holding…' : 'Press and hold to seal'}</span>
				</button>
			{:else}
				<p class="after">Sealed. You have 2 minutes to change your mind, then it’s binding.</p>
			{/if}
		</section>
	{/if}
</div>

<style>
	.wizard {
		min-height: 100%;
		padding: 0 24px 64px;
		background: var(--bg);
	}
	header {
		position: sticky;
		top: 0;
		z-index: 2;
		display: flex;
		align-items: center;
		justify-content: space-between;
		max-width: 860px;
		margin: 0 auto;
		padding: 18px 0;
		background: var(--bg);
	}
	ol {
		display: flex;
		gap: 22px;
		margin: 0;
		padding: 0;
		list-style: none;
		font-size: 12.5px;
		color: var(--faint);
	}
	li {
		display: flex;
		align-items: center;
		gap: 8px;
	}
	li span {
		display: grid;
		place-items: center;
		width: 20px;
		height: 20px;
		border-radius: 50%;
		background: var(--raised);
		font-size: 11px;
		font-weight: 700;
	}
	li.now {
		color: var(--text);
	}
	li.now span {
		background: var(--ember);
		color: #1a0b05;
	}
	li.done span {
		background: var(--ember-soft);
		color: var(--ember);
	}
	section {
		max-width: 760px;
		margin: 28px auto 0;
	}
	h1 {
		display: flex;
		align-items: center;
		gap: 12px;
		margin: 0;
		font-size: 30px;
		letter-spacing: -0.02em;
	}
	.lede {
		margin: 8px 0 0;
		color: var(--muted);
		max-width: 560px;
	}
	.badge {
		display: grid;
		place-items: center;
		width: 38px;
		height: 38px;
		border-radius: 12px;
		background: color-mix(in srgb, var(--k) 15%, transparent);
		color: var(--k);
	}
	.kinds {
		display: grid;
		grid-template-columns: repeat(auto-fill, minmax(220px, 1fr));
		gap: 12px;
		margin-top: 28px;
	}
	.kind {
		display: grid;
		gap: 6px;
		justify-items: start;
		align-content: start;
		padding: 18px;
		border-radius: 16px;
		border: 1px solid var(--line);
		background: var(--chrome);
		text-align: left;
		cursor: pointer;
		transition:
			border-color 0.15s,
			transform 0.15s;
	}
	.kind:hover,
	.kind.on {
		border-color: var(--k);
		transform: translateY(-2px);
	}
	.kind .icon {
		display: grid;
		place-items: center;
		width: 40px;
		height: 40px;
		margin-bottom: 6px;
		border-radius: 12px;
		background: color-mix(in srgb, var(--k) 15%, transparent);
		color: var(--k);
	}
	.kind strong {
		font-size: 15px;
	}
	.kind span {
		color: var(--muted);
		font-size: 13px;
	}
	.kind em {
		margin-top: 4px;
		font-style: normal;
		font-size: 12px;
		color: var(--faint);
	}
	.field {
		margin-top: 26px;
	}
	.field.row {
		display: flex;
		flex-wrap: wrap;
		gap: 28px;
	}
	.label {
		display: block;
		margin-bottom: 10px;
		font-size: 12.5px;
		font-weight: 600;
		color: var(--muted);
	}
	.sites {
		display: flex;
		flex-wrap: wrap;
		align-items: center;
		gap: 6px;
		min-height: 46px;
		padding: 6px;
		border-radius: 12px;
		border: 1px solid var(--line-2);
		background: var(--chrome);
	}
	.sites:focus-within {
		border-color: var(--ember);
	}
	.site {
		display: inline-flex;
		align-items: center;
		gap: 7px;
		height: 32px;
		padding: 0 6px 0 10px;
		border-radius: 8px;
		background: var(--raised);
		font-size: 13px;
	}
	.site button {
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
	.site button:hover {
		background: var(--raised-2);
		color: var(--text);
	}
	.sites form {
		flex: 1;
		min-width: 200px;
	}
	.sites input {
		width: 100%;
		height: 32px;
		padding: 0 8px;
		border: 0;
		outline: 0;
		background: none;
		color: var(--text);
	}
	.suggest {
		display: flex;
		flex-wrap: wrap;
		gap: 6px;
		margin-top: 10px;
	}
	.suggest button,
	.prompts button {
		display: inline-flex;
		align-items: center;
		gap: 4px;
		height: 28px;
		padding: 0 10px;
		border-radius: 999px;
		border: 1px dashed var(--line-2);
		background: none;
		color: var(--muted);
		font-size: 12px;
		cursor: pointer;
	}
	.suggest button:hover,
	.prompts button:hover {
		border-style: solid;
		color: var(--text);
	}
	.seg,
	.days {
		display: inline-flex;
		flex-wrap: wrap;
		gap: 4px;
		padding: 4px;
		border-radius: 12px;
		background: var(--chrome);
		border: 1px solid var(--line);
	}
	.seg button,
	.days button {
		height: 32px;
		padding: 0 14px;
		border: 0;
		border-radius: 8px;
		background: none;
		color: var(--muted);
		font-size: 13px;
		font-weight: 550;
		cursor: pointer;
	}
	.seg button:hover,
	.days button:hover {
		color: var(--text);
	}
	.seg button.on,
	.days button.on {
		background: var(--raised-2);
		color: var(--text);
		box-shadow: inset 0 0 0 1px var(--line-2);
	}
	.days {
		display: flex;
		width: fit-content;
		margin-top: 10px;
	}
	.days button {
		width: 48px;
		padding: 0;
	}
	.times {
		display: flex;
		gap: 14px;
		margin-top: 10px;
		color: var(--muted);
		font-size: 13px;
	}
	.times input {
		margin-left: 6px;
		height: 34px;
		padding: 0 10px;
		border-radius: 8px;
		border: 1px solid var(--line-2);
		background: var(--chrome);
		color: var(--text);
	}
	.escapes {
		display: grid;
		grid-template-columns: 1fr 1fr;
		gap: 10px;
	}
	.escapes button {
		display: grid;
		gap: 4px;
		padding: 14px 16px;
		border-radius: 12px;
		border: 1px solid var(--line);
		background: var(--chrome);
		text-align: left;
		cursor: pointer;
	}
	.escapes button.on {
		border-color: var(--ember);
		background: var(--ember-soft);
	}
	.escapes span {
		color: var(--muted);
		font-size: 12.5px;
	}
	.next {
		display: flex;
		align-items: center;
		justify-content: space-between;
		gap: 20px;
		margin-top: 34px;
		padding-top: 20px;
		border-top: 1px solid var(--line);
	}
	.sentence {
		margin: 0;
		font-size: 15px;
	}
	.count {
		color: var(--faint);
		font-size: 12px;
	}
	textarea {
		width: 100%;
		margin-top: 24px;
		padding: 18px 20px;
		border-radius: 14px;
		border: 1px solid var(--line-2);
		background: var(--chrome);
		color: var(--text);
		font-family: 'Iowan Old Style', 'Palatino Linotype', Georgia, serif;
		font-size: 22px;
		font-style: italic;
		line-height: 1.4;
		resize: none;
		outline: 0;
	}
	textarea:focus {
		border-color: var(--ember);
	}
	.prompts {
		display: flex;
		flex-wrap: wrap;
		gap: 6px;
		margin-top: 12px;
	}

	/* ---- seal */
	.sealing {
		display: flex;
		flex-direction: column;
		align-items: center;
		max-width: 560px;
	}
	.contract {
		position: relative;
		width: 100%;
		padding: 28px 30px;
		border-radius: 18px;
		background: linear-gradient(180deg, #1c1d21, #16171a);
		border: 1px solid var(--line-2);
		box-shadow: 0 30px 80px rgb(0 0 0 / 0.45);
		transition: transform 0.4s;
	}
	.contract.sealed {
		animation: thud 0.5s cubic-bezier(0.3, 1.6, 0.5, 1);
	}
	@keyframes thud {
		30% {
			transform: scale(0.98);
		}
	}
	.contract-head {
		display: flex;
		align-items: center;
		gap: 10px;
	}
	.terms {
		margin: 18px 0 0;
		padding-right: 96px;
		font-size: 19px;
		font-weight: 600;
		line-height: 1.35;
		letter-spacing: -0.01em;
	}
	dl {
		display: grid;
		grid-template-columns: repeat(3, 1fr);
		gap: 12px;
		margin: 20px 0 0;
	}
	dt {
		font-size: 11.5px;
		color: var(--faint);
	}
	dd {
		margin: 2px 0 0;
		font-size: 13px;
	}
	blockquote {
		margin: 22px 0 0;
		padding: 0 0 0 14px;
		border-left: 2px solid var(--ember);
		font-family: 'Iowan Old Style', 'Palatino Linotype', Georgia, serif;
		font-style: italic;
		font-size: 18px;
	}
	.fine {
		margin: 22px 0 0;
		padding: 16px 0 0 18px;
		border-top: 1px dashed var(--line-2);
		color: var(--muted);
		font-size: 12.5px;
		display: grid;
		gap: 4px;
	}
	.stamp {
		position: absolute;
		right: 26px;
		top: 22px;
		display: grid;
		place-items: center;
		width: 92px;
		height: 92px;
		border-radius: 50%;
		background: radial-gradient(circle at 35% 30%, #ff8f5e, #c7361a 70%);
		color: #2a0c03;
		box-shadow:
			0 0 0 5px rgb(199 54 26 / 0.35),
			0 12px 30px rgb(199 54 26 / 0.45);
		transform: rotate(-12deg);
		animation: stamp 0.45s cubic-bezier(0.2, 1.4, 0.4, 1);
		font-weight: 800;
		font-size: 12px;
		letter-spacing: 0.12em;
		text-transform: uppercase;
		align-content: center;
		gap: 2px;
	}
	@keyframes stamp {
		from {
			transform: rotate(-30deg) scale(2.2);
			opacity: 0;
		}
	}
	.phrase {
		display: grid;
		gap: 8px;
		width: 100%;
		margin-top: 28px;
		color: var(--muted);
		font-size: 13px;
		text-align: center;
	}
	.phrase b {
		color: var(--text);
	}
	.phrase input {
		height: 44px;
		padding: 0 14px;
		border-radius: 12px;
		border: 1px solid var(--line-2);
		background: var(--chrome);
		color: var(--text);
		text-align: center;
		font-size: 15px;
		outline: 0;
	}
	.phrase input:focus {
		border-color: var(--ember);
	}
	.hold {
		position: relative;
		width: 100%;
		height: 54px;
		margin-top: 12px;
		border: 1px solid var(--line-2);
		border-radius: 14px;
		background: var(--raised);
		overflow: hidden;
		cursor: not-allowed;
		font-weight: 700;
		font-size: 15px;
		user-select: none;
		touch-action: none;
	}
	.hold.ready {
		cursor: pointer;
		border-color: var(--ember);
	}
	.fill {
		position: absolute;
		inset: 0;
		background: var(--ember);
		transform-origin: left;
		transform: scaleX(var(--p));
	}
	.text {
		position: relative;
		display: inline-flex;
		align-items: center;
		gap: 8px;
		color: var(--text);
		mix-blend-mode: difference;
	}
	.hold:disabled .text {
		opacity: 0.4;
	}
	.after {
		margin-top: 24px;
		color: var(--muted);
	}
</style>
