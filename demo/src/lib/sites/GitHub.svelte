<script lang="ts">
	import type { SiteProps } from './types';
	import { gradientFor, hash } from './types';

	let { url }: SiteProps = $props();

	type Repo = {
		owner: string;
		name: string;
		description: string;
		lang: string;
		langColor: string;
		stars: number;
		forks: number;
		watchers: number;
		issues: number;
		prs: number;
	};
	type Row = { kind: 'dir' | 'file'; name: string; message: string; age: string };
	type Block = { kind: 'h1' | 'h2' | 'p' | 'code'; text: string };

	const MARK =
		'M8 0C3.58 0 0 3.58 0 8c0 3.54 2.29 6.53 5.47 7.59.4.07.55-.17.55-.38 0-.19-.01-.82-.01-1.49-2.01.37-2.53-.49-2.69-.94-.09-.23-.48-.94-.82-1.13-.28-.15-.68-.52-.01-.53.63-.01 1.08.58 1.23.82.72 1.21 1.87.87 2.33.66.07-.52.28-.87.51-1.07-1.78-.2-3.64-.89-3.64-3.95 0-.87.31-1.59.82-2.15-.08-.2-.36-1.02.08-2.12 0 0 .67-.21 2.2.82.64-.18 1.32-.27 2-.27.68 0 1.36.09 2 .27 1.53-1.04 2.2-.82 2.2-.82.44 1.1.16 1.92.08 2.12.51.56.82 1.27.82 2.15 0 3.07-1.87 3.75-3.65 3.95.29.25.54.73.54 1.48 0 1.07-.01 1.93-.01 2.2 0 .21.15.46.55.38A8.013 8.013 0 0016 8c0-4.42-3.58-8-8-8z';
	const STAR = 'M8 .25l2.39 4.84 5.34.78-3.87 3.77.91 5.32L8 12.4l-4.77 2.56.91-5.32L.27 5.87l5.34-.78z';
	const FOLDER =
		'M1.75 1A1.75 1.75 0 0 0 0 2.75v10.5C0 14.216.784 15 1.75 15h12.5A1.75 1.75 0 0 0 16 13.25v-8.5A1.75 1.75 0 0 0 14.25 3H7.5a.25.25 0 0 1-.2-.1l-.9-1.2C6.07 1.26 5.55 1 5 1H1.75Z';
	const FILE =
		'M2 1.75C2 .784 2.784 0 3.75 0h6.586c.464 0 .909.184 1.237.513l2.914 2.914c.329.328.513.773.513 1.237v9.586A1.75 1.75 0 0 1 13.25 16h-9.5A1.75 1.75 0 0 1 2 14.25Zm1.75-.25a.25.25 0 0 0-.25.25v12.5c0 .138.112.25.25.25h9.5a.25.25 0 0 0 .25-.25V6h-2.75A1.75 1.75 0 0 1 9 4.25V1.5Z';

	const REPOS: Repo[] = [
		{ owner: 'mast-browser', name: 'mast', description: 'A productivity browser that quietly removes what you do not need.', lang: 'C++', langColor: '#f34b7d', stars: 2318, forks: 141, watchers: 62, issues: 12, prs: 3 },
		{ owner: 'sveltejs', name: 'kit', description: 'web development, streamlined', lang: 'JavaScript', langColor: '#f1e05a', stars: 19870, forks: 1904, watchers: 301, issues: 214, prs: 58 },
		{ owner: 'brave', name: 'brave-core', description: 'Brave browser core features and services.', lang: 'C++', langColor: '#f34b7d', stars: 7012, forks: 1288, watchers: 210, issues: 1480, prs: 96 },
		{ owner: 'imputnet', name: 'helium', description: 'A lightweight Chromium-based browser focused on privacy and speed.', lang: 'TypeScript', langColor: '#3178c6', stars: 1544, forks: 97, watchers: 40, issues: 33, prs: 7 }
	];
	const FALLBACK_LANGS: [string, string][] = [
		['Rust', '#dea584'],
		['Go', '#00add8'],
		['Python', '#3572a5'],
		['Svelte', '#ff3e00']
	];

	const MAST_FILES: Row[] = [
		{ kind: 'dir', name: 'chrome', message: 'Sync chrome/browser/mast with upstream 131.0.6778', age: '2 hours ago' },
		{ kind: 'dir', name: 'docs', message: 'Document the tab memory retention policy', age: '3 days ago' },
		{ kind: 'dir', name: 'patches', message: 'Carry patch for tab strip reflow on resize', age: '5 days ago' },
		{ kind: 'dir', name: 'tools', message: 'Add clean-pact linter for rule files', age: '1 week ago' },
		{ kind: 'dir', name: '.github', message: 'Add CI matrix for macOS arm64', age: '2 weeks ago' },
		{ kind: 'file', name: '.gitignore', message: 'Ignore out/ and local gclient state', age: '6 months ago' },
		{ kind: 'file', name: '.gn', message: 'Enable mast_build flag by default', age: '3 weeks ago' },
		{ kind: 'file', name: 'DEPS', message: 'Roll chromium to 131.0.6778.85', age: '2 hours ago' },
		{ kind: 'file', name: 'LICENSE', message: 'Initial commit', age: '8 months ago' },
		{ kind: 'file', name: 'README.md', message: 'Rewrite build instructions for the new toolchain', age: '4 days ago' }
	];
	const GENERIC_FILES: Row[] = [
		{ kind: 'dir', name: '.github', message: 'Bump actions to current major versions', age: '2 weeks ago' },
		{ kind: 'dir', name: 'docs', message: 'Clarify contribution guidelines', age: '5 days ago' },
		{ kind: 'dir', name: 'packages', message: 'Release 1.4.2', age: '1 day ago' },
		{ kind: 'dir', name: 'scripts', message: 'Speed up local test runner', age: '3 weeks ago' },
		{ kind: 'dir', name: 'src', message: 'Fix race in the request queue', age: '2 days ago' },
		{ kind: 'dir', name: 'tests', message: 'Add regression test for empty payloads', age: '4 days ago' },
		{ kind: 'file', name: '.gitignore', message: 'Ignore build output', age: '1 year ago' },
		{ kind: 'file', name: 'LICENSE', message: 'Initial commit', age: '2 years ago' },
		{ kind: 'file', name: 'package.json', message: 'Bump dependencies', age: '6 days ago' },
		{ kind: 'file', name: 'README.md', message: 'Update install instructions', age: '3 weeks ago' }
	];

	const MAST_README: Block[] = [
		{ kind: 'h1', text: 'Mast' },
		{ kind: 'p', text: 'Mast is a productivity browser built on Chromium. It strips the parts of the modern web that compete for your attention and keeps the parts you came for.' },
		{ kind: 'h2', text: 'Features' },
		{ kind: 'p', text: 'Clean pacts remove feeds, shorts, recommendations and comments from the sites you choose, with per-site rules.' },
		{ kind: 'p', text: 'Tab memory keeps a note of why each tab was opened and brings it back when you need it again.' },
		{ kind: 'h2', text: 'Building' },
		{ kind: 'p', text: 'Mast tracks upstream Chromium through the patches directory. To fetch sources and build a debug binary:' },
		{ kind: 'code', text: 'gclient sync --with_branch_heads\ngn gen out/Default\nautoninja -C out/Default chrome' },
		{ kind: 'h2', text: 'Contributing' },
		{ kind: 'p', text: 'Read docs first, then open an issue before starting large changes. Keep patches small and rebase them on every Chromium roll.' }
	];

	const FEED: { user: string; verb: string; repo: string; detail: string; age: string }[] = [
		{ user: 'marinalo', verb: 'starred', repo: 'sveltejs/kit', detail: 'web development, streamlined', age: '1 hour ago' },
		{ user: 'mast-browser', verb: 'released', repo: 'mast-browser/mast', detail: 'v0.9.2: Clean pacts accept per-path rules. Fixes tab restore after a crash on macOS.', age: '3 hours ago' },
		{ user: 'nkoyama', verb: 'forked', repo: 'brave/brave-core', detail: 'Forked to nkoyama/brave-core', age: '5 hours ago' },
		{ user: 'orla-dev', verb: 'starred', repo: 'imputnet/helium', detail: 'A lightweight Chromium-based browser focused on privacy and speed.', age: 'yesterday' },
		{ user: 'sveltejs', verb: 'released', repo: 'sveltejs/kit', detail: '2.31.0: Faster prerendering and better error pages for route groups.', age: '2 days ago' },
		{ user: 'tomasz', verb: 'forked', repo: 'mast-browser/mast', detail: 'Forked to tomasz/mast', age: '3 days ago' }
	];

	const CHANGES: { repo: string; title: string; age: string }[] = [
		{ repo: 'mast-browser/mast', title: 'Merge pull request #142 from mast-browser/tab-memory', age: '2 hours ago' },
		{ repo: 'sveltejs/kit', title: 'fix: keep scroll position on hash navigation', age: '4 hours ago' },
		{ repo: 'mast-browser/mast', title: 'Fix Clean pact race when the feed loads late', age: '5 hours ago' },
		{ repo: 'imputnet/helium', title: 'chore: bump Chromium base to 131', age: 'yesterday' },
		{ repo: 'brave/brave-core', title: 'Add tab group sync for profiles with sync disabled', age: 'yesterday' },
		{ repo: 'mast-browser/mast', title: 'Update CHANGELOG for 0.9.2', age: '2 days ago' }
	];

	function findRepo(owner: string, name: string): Repo {
		const known = REPOS.find((r) => r.owner === owner && r.name === name);
		if (known) return known;
		const h = hash(owner + '/' + name);
		const [lang, langColor] = FALLBACK_LANGS[h % FALLBACK_LANGS.length];
		return {
			owner,
			name,
			description: `Simulated repository ${owner}/${name}.`,
			lang,
			langColor,
			stars: h % 900,
			forks: (h >> 4) % 120,
			watchers: (h >> 8) % 60,
			issues: (h >> 3) % 40,
			prs: (h >> 6) % 9
		};
	}

	function genericReadme(repo: Repo): Block[] {
		return [
			{ kind: 'h1', text: repo.name },
			{ kind: 'p', text: repo.description },
			{ kind: 'h2', text: 'Getting started' },
			{ kind: 'code', text: `git clone https://github.com/${repo.owner}/${repo.name}.git\ncd ${repo.name}\nnpm install` },
			{ kind: 'h2', text: 'License' },
			{ kind: 'p', text: 'See LICENSE for details.' }
		];
	}

	function fmt(n: number): string {
		return n >= 1000 ? (n / 1000).toFixed(1).replace(/\.0$/, '') + 'k' : String(n);
	}

	const parts = $derived(url.pathname.split('/').filter(Boolean));
	const repo = $derived(parts.length >= 2 ? findRepo(parts[0], parts[1]) : null);
	const isMast = $derived(repo?.owner === 'mast-browser' && repo?.name === 'mast');
	const files = $derived(isMast ? MAST_FILES : GENERIC_FILES);
	const readme = $derived(repo ? (isMast ? MAST_README : genericReadme(repo)) : []);
</script>

{#snippet star()}
	<svg viewBox="0 0 16 16" width="12" height="12" aria-hidden="true"><path d={STAR} fill="currentColor" /></svg>
{/snippet}

{#snippet header()}
	<header class="top">
		<a class="octo" href="/" aria-label="GitHub home">
			<svg viewBox="0 0 16 16" width="32" height="32" aria-hidden="true"><path d={MARK} fill="currentColor" /></svg>
		</a>
		<nav class="hnav">
			<a href="/pulls">Pull requests</a>
			<a href="/issues">Issues</a>
			<a href="/codespaces">Codespaces</a>
			<a href="/marketplace">Marketplace</a>
			<a href="/explore">Explore</a>
		</nav>
		<input class="search" type="search" placeholder="Type / to search" aria-label="Search" />
		<span class="avatar" style="background: {gradientFor('you')}">Y</span>
	</header>
{/snippet}

<div class="gh">
	{@render header()}

	{#if repo}
		<div class="wrap">
			<div class="rhead">
				<div class="title">
					<svg viewBox="0 0 16 16" width="16" height="16" aria-hidden="true" class="ico"><path d={FOLDER} fill="currentColor" /></svg>
					<a href="/{repo.owner}">{repo.owner}</a>
					<span class="sl">/</span>
					<a class="rn" href="/{repo.owner}/{repo.name}">{repo.name}</a>
					<span class="badge">Public</span>
				</div>
				<div class="actions">
					<button class="btn">Watch <span class="cnt">{fmt(repo.watchers)}</span></button>
					<button class="btn">Fork <span class="cnt">{fmt(repo.forks)}</span></button>
					<button class="btn">{@render star()} Star <span class="cnt">{fmt(repo.stars)}</span></button>
				</div>
			</div>

			<nav class="tabs">
				<a class="tab on" href="/{repo.owner}/{repo.name}">Code</a>
				<a class="tab" href="/{repo.owner}/{repo.name}/issues">Issues <span class="cnt">{repo.issues}</span></a>
				<a class="tab" href="/{repo.owner}/{repo.name}/pulls">Pull requests <span class="cnt">{repo.prs}</span></a>
				<a class="tab" href="/{repo.owner}/{repo.name}/actions">Actions</a>
				<a class="tab" href="/{repo.owner}/{repo.name}/wiki">Wiki</a>
			</nav>

			<div class="bar">
				<button class="btn branch">main <span class="caret">&#9662;</span></button>
				<div class="bar-r">
					<button class="btn">Go to file</button>
					<button class="btn green">Code</button>
				</div>
			</div>

			<div class="files">
				<div class="lastc">
					<span class="avatar sm" style="background: {gradientFor(repo.owner)}">{repo.owner.charAt(0).toUpperCase()}</span>
					<a href="/{repo.owner}">{repo.owner}</a>
					<span>{files[0].message}</span>
					<span class="fage">{files[0].age}</span>
				</div>
				{#each files as row (row.name)}
					<div class="frow">
						<svg viewBox="0 0 16 16" width="16" height="16" aria-hidden="true" class="ic">
							<path d={row.kind === 'dir' ? FOLDER : FILE} fill="currentColor" />
						</svg>
						<a class="fname" href="/{repo.owner}/{repo.name}/{row.kind === 'dir' ? 'tree' : 'blob'}/main/{row.name}">{row.name}</a>
						<span class="fmsg">{row.message}</span>
						<span class="fage">{row.age}</span>
					</div>
				{/each}
			</div>

			<section class="readme">
				<div class="rh">README.md</div>
				<div class="rb">
					{#each readme as b, i (i)}
						{#if b.kind === 'h1'}
							<h1>{b.text}</h1>
						{:else if b.kind === 'h2'}
							<h2>{b.text}</h2>
						{:else if b.kind === 'code'}
							<pre><code>{b.text}</code></pre>
						{:else}
							<p>{b.text}</p>
						{/if}
					{/each}
				</div>
			</section>
		</div>
	{:else}
		<div class="dash">
			<aside class="panel">
				<h2 class="ph">Top repositories</h2>
				<ul class="repos">
					{#each REPOS as r (r.owner + '/' + r.name)}
						<li>
							<span class="dot" style="background: {r.langColor}"></span>
							<a class="rname" href="/{r.owner}/{r.name}">{r.owner}/{r.name}</a>
							<span class="stat">{@render star()} {fmt(r.stars)}</span>
						</li>
					{/each}
				</ul>
			</aside>

			<main class="feed">
				<h2 class="fh">Home</h2>
				{#each FEED as f, i (i)}
					<article class="card">
						<div class="ch">
							<span class="avatar sm" style="background: {gradientFor(f.user)}">{f.user.charAt(0).toUpperCase()}</span>
							<b>{f.user}</b>
							{f.verb}
							<a href="/{f.repo}">{f.repo}</a>
							<span class="age">{f.age}</span>
						</div>
						<p class="cd">{f.detail}</p>
					</article>
				{/each}
			</main>

			<aside class="panel">
				<h2 class="ph">Latest changes</h2>
				<ul class="chg">
					{#each CHANGES as c, i (i)}
						<li>
							<a href="/{c.repo}">{c.title}</a>
							<span class="cm">{c.repo} &middot; {c.age}</span>
						</li>
					{/each}
				</ul>
			</aside>
		</div>
	{/if}
</div>

<style>
	.gh { min-height: 100%; background: #0d1117; color: #e6edf3; font-family: -apple-system, 'Segoe UI', 'Noto Sans', Helvetica, Arial, sans-serif; font-size: 14px; line-height: 1.5; }
	a { color: #4493f8; text-decoration: none; }
	a:hover { text-decoration: underline; }
	button { font: inherit; color: inherit; }
	.top { display: flex; align-items: center; gap: 16px; padding: 10px 16px; background: #161b22; border-bottom: 1px solid #30363d; }
	.octo { display: flex; color: #e6edf3; }
	.octo:hover { text-decoration: none; }
	.hnav { display: flex; gap: 16px; font-size: 14px; font-weight: 600; }
	.hnav a { color: #e6edf3; }
	.search { margin-left: auto; width: 240px; padding: 5px 12px; border: 1px solid #30363d; border-radius: 6px; background: #0d1117; color: #e6edf3; font: inherit; font-size: 14px; }
	.search::placeholder { color: #7d8590; }
	.avatar { display: grid; place-items: center; width: 28px; height: 28px; flex: none; border-radius: 50%; color: #fff; font-size: 12px; font-weight: 600; }
	.avatar.sm { width: 20px; height: 20px; font-size: 10px; }
	.panel { background: #161b22; border: 1px solid #30363d; border-radius: 6px; }
	.ph { margin: 0; padding: 12px 16px; border-bottom: 1px solid #30363d; font-size: 14px; font-weight: 600; }
	.dash { display: grid; grid-template-columns: 296px minmax(0, 1fr) 296px; align-items: start; gap: 16px; padding: 16px; }
	.repos, .chg { list-style: none; margin: 0; padding: 4px 0; }
	.repos li { display: flex; align-items: center; gap: 8px; padding: 6px 16px; }
	.rname { min-width: 0; flex: 1; overflow: hidden; font-weight: 600; text-overflow: ellipsis; white-space: nowrap; }
	.dot { width: 10px; height: 10px; flex: none; border-radius: 50%; }
	.stat { display: flex; align-items: center; gap: 3px; color: #7d8590; font-size: 12px; }
	.feed { display: flex; flex-direction: column; gap: 12px; min-width: 0; }
	.fh { margin: 0; font-size: 20px; font-weight: 600; }
	.card { padding: 14px 16px; background: #161b22; border: 1px solid #30363d; border-radius: 6px; }
	.ch { display: flex; flex-wrap: wrap; align-items: center; gap: 6px; color: #7d8590; font-size: 13px; }
	.ch b { color: #e6edf3; }
	.age { margin-left: auto; font-size: 12px; }
	.cd { margin: 8px 0 0 26px; color: #9198a1; font-size: 13px; }
	.chg li { display: flex; flex-direction: column; gap: 2px; padding: 8px 16px; border-top: 1px solid #21262d; }
	.chg li:first-child { border-top: none; }
	.cm { color: #7d8590; font-size: 12px; }
	.wrap { max-width: 1200px; margin: 0 auto; padding: 16px 24px 40px; }
	.rhead { display: flex; flex-wrap: wrap; align-items: center; justify-content: space-between; gap: 12px; }
	.title { display: flex; align-items: center; gap: 8px; font-size: 20px; }
	.ico { color: #7d8590; }
	.sl { color: #7d8590; }
	.rn { font-weight: 600; }
	.badge { padding: 0 7px; border: 1px solid #30363d; border-radius: 2em; color: #7d8590; font-size: 12px; font-weight: 500; line-height: 18px; }
	.actions, .bar-r { display: flex; gap: 8px; }
	.btn { display: inline-flex; align-items: center; gap: 6px; padding: 5px 12px; border: 1px solid #363b42; border-radius: 6px; background: #21262d; font-size: 12px; font-weight: 500; cursor: pointer; }
	.btn.green { border-color: rgb(240 246 252 / 0.1); background: #238636; color: #fff; }
	.btn .cnt { padding: 0 7px; border-radius: 2em; background: rgb(240 246 252 / 0.1); font-size: 12px; }
	.tabs { display: flex; gap: 4px; margin-top: 16px; border-bottom: 1px solid #30363d; overflow-x: auto; }
	.tab { display: flex; align-items: center; gap: 6px; padding: 8px 12px; border-bottom: 2px solid transparent; color: #e6edf3; white-space: nowrap; }
	.tab:hover { border-bottom-color: #6e7681; text-decoration: none; }
	.tab.on { border-bottom-color: #f78166; font-weight: 600; }
	.tab .cnt { padding: 0 6px; border-radius: 2em; background: #30363d; font-size: 12px; }
	.bar { display: flex; align-items: center; justify-content: space-between; gap: 8px; margin: 16px 0; }
	.branch { padding: 5px 12px; font-size: 12px; font-weight: 600; }
	.caret { color: #7d8590; }
	.files { overflow: hidden; border: 1px solid #30363d; border-radius: 6px; }
	.lastc { display: flex; align-items: center; gap: 8px; padding: 8px 16px; background: #161b22; border-bottom: 1px solid #30363d; color: #9198a1; font-size: 12px; }
	.lastc a { color: #e6edf3; font-weight: 600; }
	.frow { display: grid; grid-template-columns: 16px minmax(120px, 200px) minmax(0, 1fr) auto; align-items: center; gap: 12px; padding: 6px 16px; border-top: 1px solid #21262d; font-size: 14px; }
	.ic { color: #7d8590; }
	.fname { overflow: hidden; color: #e6edf3; text-overflow: ellipsis; white-space: nowrap; }
	.fname:hover { color: #4493f8; }
	.fmsg { overflow: hidden; color: #9198a1; text-overflow: ellipsis; white-space: nowrap; }
	.fage { color: #7d8590; font-size: 12px; white-space: nowrap; }
	.readme { margin-top: 16px; overflow: hidden; border: 1px solid #30363d; border-radius: 6px; }
	.rh { padding: 8px 16px; background: #161b22; border-bottom: 1px solid #30363d; font-size: 12px; font-weight: 600; }
	.rb { padding: 24px 32px; font-size: 16px; line-height: 1.5; }
	.rb h1 { margin: 0 0 16px; padding-bottom: 0.3em; border-bottom: 1px solid #30363d; font-size: 2em; font-weight: 600; }
	.rb h2 { margin: 24px 0 16px; padding-bottom: 0.3em; border-bottom: 1px solid #30363d; font-size: 1.5em; font-weight: 600; }
	.rb p { margin: 0 0 16px; }
	.rb pre { overflow: auto; margin: 0 0 16px; padding: 16px; border-radius: 6px; background: #161b22; font-family: ui-monospace, SFMono-Regular, Consolas, monospace; font-size: 85%; line-height: 1.45; }
</style>
