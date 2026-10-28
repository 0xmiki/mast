<script lang="ts">
	import type { SiteProps } from './types';
	import { hash } from './types';
	import CleanedNotice from './CleanedNotice.svelte';

	let { url, clean, navigate }: SiteProps = $props();

	type Story = {
		id: number;
		title: string;
		/** Empty for text posts such as Ask HN, which show no domain. */
		domain: string;
		points: number;
		user: string;
		hours: number;
		comments: number;
	};

	type Comment = { user: string; hours: number; depth: number; text: string };

	const STORIES: Story[] = [
		{ id: 41031, title: 'Show HN: I built a tool that turns git history into a sound you can hear', domain: 'soundofcommits.dev', points: 412, user: 'quietbyte', hours: 3, comments: 187 },
		{ id: 41032, title: 'Ask HN: How do you keep side projects alive after the first month?', domain: '', points: 268, user: 'tinkerwell', hours: 5, comments: 214 },
		{ id: 41033, title: 'The Ulysses pact and precommitment devices', domain: 'tinyessays.net', points: 233, user: 'ferrousmoss', hours: 2, comments: 96 },
		{ id: 41034, title: 'Chromium fork maintenance is harder than you think', domain: 'fromthetrenches.dev', points: 219, user: 'vendoredlib', hours: 7, comments: 143 },
		{ id: 41035, title: 'Why SQLite still beats most distributed databases for small teams', domain: 'sqlite-or-not.io', points: 198, user: 'pagesize', hours: 4, comments: 131 },
		{ id: 41036, title: 'A retrospective on rewriting our build system in Rust', domain: 'buildlog.dev', points: 176, user: 'hermeticyak', hours: 9, comments: 88 },
		{ id: 41037, title: 'Show HN: Flashcards that schedule themselves with a tiny neural net', domain: 'recall-lab.app', points: 164, user: 'spacedrepeat', hours: 1, comments: 57 },
		{ id: 41038, title: 'Subpixel text rendering: why your fonts look different on every screen', domain: 'fontgeist.dev', points: 158, user: 'kerning_kid', hours: 6, comments: 72 },
		{ id: 41039, title: 'The quiet collapse of the regional newspaper, in charts', domain: 'datadesk-notes.org', points: 151, user: 'orbitalsub', hours: 11, comments: 169 },
		{ id: 41040, title: 'Mesh networking from scratch in 400 lines of Go', domain: 'wiregrove.dev', points: 142, user: 'nodeforge', hours: 8, comments: 39 },
		{ id: 41041, title: 'Ask HN: Who is still running a Gopher server, and why?', domain: '', points: 129, user: 'oldnet', hours: 3, comments: 102 },
		{ id: 41042, title: 'Oxide and the case for boring hardware', domain: 'oxcart.systems', points: 121, user: 'racktop', hours: 12, comments: 64 },
		{ id: 41043, title: 'The 10x engineer myth, revisited with actual data', domain: 'softwarestats.blog', points: 117, user: 'medianteam', hours: 5, comments: 245 },
		{ id: 41044, title: 'Show HN: A terminal pet that reacts to your failing test suite', domain: 'pawsterm.dev', points: 109, user: 'cronpaws', hours: 2, comments: 44 },
		{ id: 41045, title: 'Linux 7.2 adds a scheduler tuned for latency-sensitive desktops', domain: 'kerneltimes.net', points: 104, user: 'preemptive', hours: 10, comments: 78 },
		{ id: 41046, title: 'A short history of the mass-produced mechanical watch', domain: 'horology-notes.com', points: 98, user: 'escapement', hours: 14, comments: 33 },
		{ id: 41047, title: 'Why I stopped using a framework for my blog, and what I use now', domain: 'notesfrom.work', points: 93, user: 'plainhtml', hours: 4, comments: 61 },
		{ id: 41048, title: 'Show HN: Open-source notebooks for reproducible papers', domain: 'papernb.org', points: 88, user: 'reprodux', hours: 6, comments: 27 },
		{ id: 41049, title: 'The economics of open-source maintainer burnout', domain: 'opensourcegrind.org', points: 84, user: 'stewardship', hours: 9, comments: 118 },
		{ id: 41050, title: 'How people actually read long PDFs: an eye-tracking study', domain: 'readinglab.edu', points: 79, user: 'saccade', hours: 16, comments: 41 },
		{ id: 41051, title: 'Writing a CPU emulator in one weekend, and what I learned', domain: 'emulate-this.dev', points: 76, user: 'opcodes', hours: 7, comments: 29 },
		{ id: 41052, title: 'Ask HN: What is the most underrated feature in your text editor?', domain: '', points: 71, user: 'bufferbob', hours: 2, comments: 193 },
		{ id: 41053, title: 'Postmortem: a 41-minute outage caused by a cron job on a leap day', domain: 'status-report.dev', points: 67, user: 'oncallnotes', hours: 13, comments: 36 },
		{ id: 41054, title: 'Designing for attention: notes on calm software', domain: 'calmtech.design', points: 61, user: 'lowcontrast', hours: 18, comments: 24 },
		{ id: 41055, title: 'Show HN: A local-first notes app that syncs over plain files', domain: 'plainsync.app', points: 58, user: 'flatfiles', hours: 5, comments: 49 },
		{ id: 41056, title: 'Every OAuth library seems to have the same three bugs', domain: 'authbugs.dev', points: 54, user: 'redirectme', hours: 21, comments: 52 },
		{ id: 41057, title: 'Perft, move generators, and the bugs that hide in chess engines', domain: 'chessprogramming.blog', points: 49, user: 'bitboard', hours: 10, comments: 17 },
		{ id: 41058, title: 'The case for a boring single-binary deploy', domain: 'deployboring.net', points: 44, user: 'onefile', hours: 15, comments: 28 },
		{ id: 41059, title: 'A spreadsheet beat our churn model, and nobody is happy about it', domain: 'spreadsheetsrule.co', points: 39, user: 'pivotcache', hours: 19, comments: 73 },
		{ id: 41060, title: 'Show HN: A bookmark manager that gives your tabs a memory', domain: 'tabmemory.app', points: 35, user: 'lastopened', hours: 1, comments: 12 }
	];

	const COMMENTS: Comment[] = [
		{ user: 'mdeng', hours: 2, depth: 0, text: 'This is a better writeup than most of what gets posted here. The part about how the failure shows up in production is the bit I wish I had read two years ago.' },
		{ user: 'pfennig', hours: 2, depth: 1, text: 'Agreed. I would add that the interesting cases are the ones where the constraint is annoying to the person who set it, because that is usually where it actually does its job.' },
		{ user: 'quietbyte', hours: 1, depth: 2, text: 'Does that hold when the constraint is set by someone else? In my experience people just route around it.' },
		{ user: 'saltmarsh', hours: 1, depth: 3, text: 'They route around it on a Tuesday and ask for it back on a Friday. Somewhere between those two days is the whole design problem.' },
		{ user: 'ottolenz', hours: 1, depth: 2, text: 'We hit a version of this last spring. The fix was embarrassingly small, and it took three weeks of meetings to find it.' },
		{ user: 'rkimura', hours: 3, depth: 0, text: 'Could someone link the original paper? The abstract makes a stronger claim than the blog post does, and I would like to see the numbers for myself.' },
		{ user: 'wendt_ada', hours: 2, depth: 1, text: 'Here it is, along with a replication from a different group. The replication is more modest, and the authors are more candid about the limits.' },
		{ user: 'hollowpine', hours: 4, depth: 0, text: 'Small nit: the second chart uses a log scale without saying so, which makes the slope look gentler than it is. Otherwise a nice piece of work.' }
	];

	const NAV = [
		{ label: 'new', href: '/newest' },
		{ label: 'past', href: '/front' },
		{ label: 'comments', href: '/newcomments' },
		{ label: 'ask', href: '/ask' },
		{ label: 'show', href: '/show' },
		{ label: 'jobs', href: '/jobs' },
		{ label: 'submit', href: '/submit' }
	];

	const FOOTER = [
		{ label: 'Guidelines', href: '/newsguidelines.html' },
		{ label: 'FAQ', href: '/newsfaq.html' },
		{ label: 'Lists', href: '/lists' },
		{ label: 'API', href: '/api.html' },
		{ label: 'Security', href: '/security.html' },
		{ label: 'Legal', href: '/legal.html' },
		{ label: 'Apply to YC', href: 'https://www.ycombinator.com/apply' },
		{ label: 'Contact', href: '/contact' }
	];

	const onItem = $derived(url.pathname === '/item');
	const itemId = $derived(Number(url.searchParams.get('id')) || 0);
	const story = $derived(
		STORIES.find((s) => s.id === itemId) ?? STORIES[hash(String(itemId)) % STORIES.length]
	);
	const feedHidden = $derived(clean.includes('feed'));
	const commentsHidden = $derived(clean.includes('comments'));

	function plural(n: number, word: string): string {
		return `${n} ${word}${n === 1 ? '' : 's'}`;
	}

	function ago(hours: number): string {
		return `${plural(hours, 'hour')} ago`;
	}

	function slugify(title: string): string {
		return title.toLowerCase().replace(/[^a-z0-9]+/g, '-').replace(/^-|-$/g, '');
	}

	function storyHref(s: Story): string {
		return s.domain ? `https://${s.domain}/${slugify(s.title)}` : `/item?id=${s.id}`;
	}

	function onSearch(e: Event) {
		e.preventDefault();
		const q = String(new FormData(e.currentTarget as HTMLFormElement).get('q') ?? '').trim();
		if (q) navigate('https://hn.algolia.com/?q=' + encodeURIComponent(q));
	}
</script>

<div class="hn">
	<div class="page">
		<header class="top">
			<a class="logo" href="/" aria-label="Hacker News">Y</a>
			<span class="name"><a href="/">Hacker News</a></span>
			<span class="links">
				{#each NAV as n, i (n.label)}
					{#if i > 0}<span class="pipe">|</span>{/if}
					<a href={n.href}>{n.label}</a>
				{/each}
			</span>
		</header>

		<main class="main">
			{#if onItem}
				<div class="item">
					<div class="titleline">
						<a class="title" href={storyHref(story)}>{story.title}</a>
						{#if story.domain}<span class="site">({story.domain})</span>{/if}
					</div>
					<div class="sub">
						{story.points} points by {story.user} {ago(story.hours)} <span class="pipe">|</span>
						{plural(story.comments, 'comment')}
					</div>
				</div>

				{#if commentsHidden}
					<CleanedNotice label="Comments" />
				{:else}
					<div class="comments">
						{#each COMMENTS as c, i (i)}
							<div class="comment" style="margin-left: {c.depth * 40}px">
								<div class="cmeta">
									<span class="cuser">{c.user}</span> {ago(c.hours)} <span class="pipe">|</span>
									<span>reply</span>
								</div>
								<p class="ctext">{c.text}</p>
							</div>
						{/each}
					</div>
				{/if}
			{:else if feedHidden}
				<CleanedNotice label="Front page" />
				<form class="search" onsubmit={onSearch}>
					<label class="sr" for="hn-q">Search Hacker News</label>
					<input id="hn-q" name="q" type="search" placeholder="Search Hacker News" autocomplete="off" />
					<button type="submit">Search</button>
				</form>
			{:else}
				<ol class="stories">
					{#each STORIES as s, i (s.id)}
						<li class="story">
							<span class="rank">{i + 1}.</span>
							<svg class="up" viewBox="0 0 10 10" aria-hidden="true"><path d="M5 1 9.5 8.5H0.5Z" /></svg>
							<div class="body">
								<div class="titleline">
									<a class="title" href="/item?id={s.id}">{s.title}</a>
									{#if s.domain}<span class="site">({s.domain})</span>{/if}
								</div>
								<div class="sub">
									{s.points} points by {s.user} {ago(s.hours)} <span class="pipe">|</span>
									{plural(s.comments, 'comment')}
								</div>
							</div>
						</li>
					{/each}
				</ol>
			{/if}
		</main>

		<footer class="foot">
			<p>
				{#each FOOTER as f, i (f.label)}
					{#if i > 0}<span class="pipe">|</span>{/if}
					<a href={f.href}>{f.label}</a>
				{/each}
			</p>
		</footer>
	</div>
</div>

<style>
	.hn {
		min-height: 100%;
		background: #f6f6ef;
		color: #000;
		font: 13px/1.4 Verdana, Geneva, sans-serif;
	}
	.page {
		width: 85%;
		margin: 0 auto;
		padding-bottom: 24px;
	}
	a {
		color: #000;
		text-decoration: none;
	}
	a:hover {
		text-decoration: underline;
	}
	.pipe {
		margin: 0 3px;
	}

	.top {
		display: flex;
		align-items: center;
		gap: 6px;
		padding: 4px 6px;
		background: #ff6600;
	}
	.logo {
		display: inline-flex;
		align-items: center;
		justify-content: center;
		box-sizing: border-box;
		width: 18px;
		height: 18px;
		border: 2px solid #fff;
		color: #fff;
		font-weight: 700;
		font-size: 12px;
		line-height: 1;
	}
	.logo:hover {
		text-decoration: none;
	}
	.name {
		font-weight: 700;
		margin-right: 6px;
	}
	.links {
		font-size: 12px;
	}

	.main {
		padding: 10px 0 0;
	}
	.stories {
		list-style: none;
		margin: 0;
		padding: 0;
	}
	.story {
		display: flex;
		align-items: flex-start;
		gap: 2px;
		padding: 2px 0 9px;
	}
	.rank {
		flex: none;
		width: 36px;
		padding-right: 4px;
		text-align: right;
		color: #828282;
		font-size: 13px;
		line-height: 1.4;
	}
	.up {
		flex: none;
		width: 10px;
		height: 10px;
		margin: 5px 6px 0 0;
		fill: #999;
	}
	.body {
		min-width: 0;
	}
	.titleline {
		line-height: 1.3;
	}
	.title {
		font-size: 13px;
	}
	.site {
		margin-left: 4px;
		color: #828282;
		font-size: 11px;
	}
	.sub {
		color: #828282;
		font-size: 11px;
		line-height: 1.6;
	}

	.item {
		padding: 6px 0 12px;
	}
	.comments {
		padding-top: 6px;
	}
	.comment {
		padding: 6px 0 4px;
	}
	.cmeta {
		color: #828282;
		font-size: 11px;
	}
	.cuser {
		color: #000;
	}
	.ctext {
		max-width: 80ch;
		margin: 4px 0 0;
		font-size: 12px;
		line-height: 1.5;
	}

	.search {
		display: flex;
		align-items: center;
		gap: 6px;
		margin-top: 12px;
		font-size: 12px;
	}
	.search input {
		flex: 1;
		max-width: 320px;
		padding: 4px 6px;
		border: 1px solid #bbb;
		border-radius: 2px;
		background: #fff;
		font: inherit;
		font-size: 12px;
	}
	.search button {
		padding: 4px 10px;
		border: 1px solid #bbb;
		border-radius: 2px;
		background: #fff;
		font: inherit;
		font-size: 12px;
		cursor: pointer;
	}
	.sr {
		position: absolute;
		width: 1px;
		height: 1px;
		overflow: hidden;
		clip: rect(0 0 0 0);
		white-space: nowrap;
	}

	.foot {
		margin-top: 20px;
		padding-top: 10px;
		border-top: 2px solid #ff6600;
		color: #000;
		font-size: 11px;
		text-align: center;
	}
	.foot p {
		margin: 0;
	}
</style>
