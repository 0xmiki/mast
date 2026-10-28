<script lang="ts">
	import { Lock, ShieldCheck, VenetianMask, Users, Code, Clock, Trash2, Puzzle, RotateCcw, MonitorSmartphone, Power } from '@lucide/svelte';
	import { browser } from '#lib/mast/browser.svelte.ts';
	import { pacts } from '#lib/mast/pacts.svelte.ts';
	import { formatDate } from '#lib/mast/engine.ts';
	import { attemptEscape } from '#lib/mast/escapes.ts';

	const locked = $derived(pacts.locked);
	const until = $derived(locked ? formatDate(Math.max(...pacts.live.map((p) => p.endsAt))) : '');
	const lockNote = $derived(`Sealed until ${until} · ${pacts.live.length} pact${pacts.live.length === 1 ? '' : 's'} in force`);

	let theme = $state('Dark');
	let homepage = $state(true);
</script>

<div class="page">
	<nav>
		<h1>Settings</h1>
		<a class="on" href="#protection">Protection</a>
		<a href="#privacy">Privacy</a>
		<a href="#system">System</a>
		<a href="#appearance">Appearance</a>
	</nav>

	<main>
		{#if locked}
			<div class="banner">
				<Lock size={18} />
				<div>
					<strong>Some settings are sealed</strong>
					<span>{lockNote}. They unlock on their own when the last pact ends.</span>
				</div>
			</div>
		{/if}

		<h2 id="protection">Protection</h2>
		<div class="card">
			<div class="row">
				<span class="ic"><ShieldCheck size={18} /></span>
				<div class="txt">
					<strong>Pact enforcement</strong>
					<span>Built into the browser. There is no switch for this, on purpose.</span>
				</div>
				<span class="chip good">Always on</span>
			</div>
			<div class="row">
				<span class="ic"><Clock size={18} /></span>
				<div class="txt">
					<strong>Trusted time</strong>
					<span>Pacts run on a monotonic clock checked against network time. System clock changes are ignored and logged.</span>
				</div>
				<span class="chip good">Always on</span>
			</div>
			<div class="row">
				<span class="ic"><Puzzle size={18} /></span>
				<div class="txt">
					<strong>Extensions can’t touch Mast</strong>
					<span>Extensions can’t read, block or restyle mast:// pages or the block screen.</span>
				</div>
				<span class="chip good">Always on</span>
			</div>
			<div class="row">
				<span class="ic"><MonitorSmartphone size={18} /></span>
				<div class="txt">
					<strong>Mast Guard <em>planned</em></strong>
					<span>Optional OS companion that closes other browsers while a pact is in force.</span>
				</div>
				<button class="btn small" onclick={() => attemptEscape('other-browser')}>Learn more</button>
			</div>
		</div>

		<h2 id="privacy">Privacy</h2>
		<div class="card">
			{#each [{ icon: VenetianMask, title: 'Incognito windows', text: 'Private browsing.', kind: 'incognito' }, { icon: Users, title: 'Guest mode & new profiles', text: 'Let someone else browse, or start a new profile.', kind: 'profile' }, { icon: Code, title: 'Developer tools', text: 'Inspect pages and run scripts.', kind: 'devtools' }] as const as item}
				<div class="row" class:sealed={locked}>
					<span class="ic"><item.icon size={18} /></span>
					<div class="txt">
						<strong>{item.title}</strong>
						<span>{locked ? (item.kind === 'devtools' ? 'Off on sealed sites while pacts are in force.' : 'Off while pacts are in force.') : item.text}</span>
					</div>
					{#if locked}
						<button class="toggle locked" aria-label="{item.title} sealed" onclick={() => attemptEscape(item.kind)}>
							<Lock size={11} strokeWidth={2.6} />
						</button>
					{:else}
						<span class="toggle on"></span>
					{/if}
				</div>
			{/each}
			<div class="row">
				<span class="ic"><Trash2 size={18} /></span>
				<div class="txt">
					<strong>Clear browsing data</strong>
					<span>History, cookies and cache. Pacts, usage and the ledger are stored separately and stay.</span>
				</div>
				<button
					class="btn small"
					onclick={() => {
						browser.clearHistory();
						browser.toast(locked ? `Cleared. ${pacts.live.length} pacts still in force.` : 'Browsing data cleared.', 'good');
					}}>Clear</button
				>
			</div>
		</div>

		<h2 id="system">System</h2>
		<div class="card">
			<div class="row" class:sealed={locked}>
				<span class="ic"><RotateCcw size={18} /></span>
				<div class="txt">
					<strong>Reset settings</strong>
					<span>{locked ? 'Sealed while pacts are in force. Resetting never removes pacts.' : 'Restore defaults. Never removes pacts.'}</span>
				</div>
				<button class="btn small" disabled={locked} onclick={() => browser.toast('Settings reset.')}>{#if locked}<Lock size={12} />{/if} Reset</button>
			</div>
			<div class="row" class:sealed={locked}>
				<span class="ic"><Power size={18} /></span>
				<div class="txt">
					<strong>Uninstall Mast</strong>
					<span>{locked ? 'Held by the system service until the last pact ends.' : 'Available whenever no pact is in force.'}</span>
				</div>
				<button class="btn small" onclick={() => attemptEscape('uninstall')}>{#if locked}<Lock size={12} />{/if} Uninstall</button>
			</div>
		</div>

		<h2 id="appearance">Appearance</h2>
		<div class="card">
			<div class="row">
				<div class="txt"><strong>Theme</strong><span>Not sealed. Pacts only lock what could break them.</span></div>
				<div class="seg">
					{#each ['Dark', 'Light', 'System'] as t}
						<button class:on={theme === t} onclick={() => (theme = t)}>{t}</button>
					{/each}
				</div>
			</div>
			<div class="row">
				<div class="txt"><strong>Show pacts on the new tab page</strong><span>Your countdowns, front and centre.</span></div>
				<button class="toggle" class:on={homepage} aria-label="Toggle" onclick={() => (homepage = !homepage)}></button>
			</div>
		</div>
	</main>
</div>

<style>
	.page {
		display: grid;
		grid-template-columns: 220px minmax(0, 720px);
		gap: 32px;
		justify-content: center;
		min-height: 100%;
		padding: 32px 24px 64px;
		background: var(--bg);
	}
	nav {
		position: sticky;
		top: 0;
		display: grid;
		align-content: start;
		gap: 2px;
	}
	nav h1 {
		margin: 0 0 14px 12px;
		font-size: 22px;
	}
	nav a {
		padding: 8px 12px;
		border-radius: 999px;
		color: var(--muted);
		text-decoration: none;
		font-size: 13.5px;
	}
	nav a:hover,
	nav a.on {
		background: var(--chrome-2);
		color: var(--text);
	}
	.banner {
		display: flex;
		gap: 14px;
		align-items: center;
		padding: 14px 18px;
		border-radius: 14px;
		background: var(--ember-soft);
		border: 1px solid rgb(255 106 61 / 0.3);
		color: var(--ember-2);
	}
	.banner div {
		display: grid;
	}
	.banner strong {
		color: var(--text);
	}
	.banner span {
		color: var(--muted);
		font-size: 13px;
	}
	h2 {
		margin: 28px 0 10px 4px;
		font-size: 15px;
	}
	.card {
		padding: 4px 0;
	}
	.row {
		display: flex;
		align-items: center;
		gap: 14px;
		padding: 14px 18px;
	}
	.row + .row {
		border-top: 1px solid var(--line);
	}
	.ic {
		color: var(--muted);
		display: grid;
	}
	.txt {
		display: grid;
		flex: 1;
		gap: 2px;
	}
	.txt strong {
		font-weight: 550;
	}
	.txt em {
		margin-left: 6px;
		padding: 1px 6px;
		border-radius: 4px;
		background: var(--raised-2);
		font-style: normal;
		font-size: 11px;
		color: var(--muted);
	}
	.txt span {
		color: var(--muted);
		font-size: 12.5px;
	}
	.row.sealed .txt strong {
		color: var(--muted);
	}
	.toggle {
		position: relative;
		width: 36px;
		height: 20px;
		border: 0;
		border-radius: 99px;
		background: var(--raised-2);
		cursor: pointer;
		flex: none;
	}
	.toggle::after {
		content: '';
		position: absolute;
		top: 3px;
		left: 3px;
		width: 14px;
		height: 14px;
		border-radius: 50%;
		background: var(--muted);
		transition: transform 0.15s;
	}
	.toggle.on {
		background: var(--ember);
	}
	.toggle.on::after {
		transform: translateX(16px);
		background: #fff;
	}
	.toggle.locked {
		display: grid;
		place-items: center;
		color: var(--ember);
		background: var(--ember-soft);
		box-shadow: inset 0 0 0 1px rgb(255 106 61 / 0.3);
	}
	.toggle.locked::after {
		display: none;
	}
	.seg {
		display: inline-flex;
		gap: 2px;
		padding: 3px;
		border-radius: 10px;
		background: var(--chrome-2);
	}
	.seg button {
		height: 28px;
		padding: 0 12px;
		border: 0;
		border-radius: 7px;
		background: none;
		color: var(--muted);
		cursor: pointer;
		font-size: 12.5px;
	}
	.seg button.on {
		background: var(--raised-2);
		color: var(--text);
	}
</style>
