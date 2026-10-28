<script lang="ts">
	import type { SiteProps } from './types';
	import { gradientFor } from './types';

	let { url }: SiteProps = $props();

	/** "www.bakery-kitchen.com" becomes "Bakery kitchen". */
	function prettify(hostname: string): string {
		const host = hostname.replace(/^www\./, '');
		const label = (host.split('.')[0] || host).replace(/[-_]+/g, ' ');
		return label.charAt(0).toUpperCase() + label.slice(1);
	}

	const host = $derived(url.hostname.replace(/^www\./, ''));
	const title = $derived(prettify(url.hostname));
	const initial = $derived(title.charAt(0).toUpperCase() || '?');

	const CARDS = [
		{
			heading: 'Overview',
			body: 'A short summary of what this site offers and who it is for, written in plain language for first-time visitors.'
		},
		{
			heading: 'Recent updates',
			body: 'The latest news, changes and announcements, listed in reverse order of publication so the newest item comes first.'
		},
		{
			heading: 'Contact',
			body: 'Ways to get in touch, including a general enquiries address, a contact form and opening hours where relevant.'
		}
	];
</script>

<div class="gen">
	<header class="bar">
		<span class="fav" style="background: {gradientFor(url.hostname)}">{initial}</span>
		<span class="host">{url.hostname}</span>
	</header>

	<section class="hero">
		<div class="banner" style="background: {gradientFor(url.hostname)}"></div>
		<div class="inner">
			<p class="eyebrow">{host}</p>
			<h1>{title}</h1>
			<p class="path">{url.pathname}</p>
		</div>
	</section>

	<main class="content">
		<p>
			Welcome to {title}. This page stands in for the live website at {host}, so the layout and words here are
			placeholders that show the kind of information a visitor would normally find.
		</p>
		<p>
			Most pages on a site like this begin with a clear introduction, followed by sections that explain what the
			organisation does, what it offers, and how to take the next step. Nothing on this page is real, and no
			information here should be relied on.
		</p>
		<p>
			Navigation works the way it does on any other page. Pick a link to move on, or use the address bar to visit a
			different address.
		</p>

		<div class="grid">
			{#each CARDS as card, i (card.heading)}
				<article class="card">
					<div class="thumb" style="background: {gradientFor(host + ':' + i)}"></div>
					<h2>{card.heading}</h2>
					<p>{card.body}</p>
				</article>
			{/each}
		</div>
	</main>

	<footer class="note">Simulated page &mdash; in the real Mast browser this is the live site.</footer>
</div>

<style>
	.gen {
		display: flex;
		flex-direction: column;
		min-height: 100%;
		background: #fafafa;
		color: #222;
		font: 16px/1.6 system-ui, -apple-system, 'Segoe UI', sans-serif;
	}

	.bar {
		display: flex;
		align-items: center;
		gap: 10px;
		padding: 12px 20px;
		border-bottom: 1px solid #e5e5e5;
		background: #fff;
	}
	.fav {
		display: grid;
		place-items: center;
		flex: none;
		width: 24px;
		height: 24px;
		border-radius: 50%;
		color: #fff;
		font-size: 12px;
		font-weight: 600;
	}
	.host {
		color: #555;
		font-size: 14px;
	}

	.hero {
		background: #fff;
		border-bottom: 1px solid #e5e5e5;
	}
	.banner {
		height: 96px;
		opacity: 0.85;
	}
	.inner {
		max-width: 760px;
		margin: 0 auto;
		padding: 24px 24px 28px;
	}
	.eyebrow {
		margin: 0 0 4px;
		color: #888;
		font-size: 13px;
		letter-spacing: 0.04em;
		text-transform: uppercase;
	}
	h1 {
		margin: 0;
		color: #111;
		font-size: 34px;
		font-weight: 650;
		line-height: 1.2;
		letter-spacing: -0.01em;
	}
	.path {
		margin: 6px 0 0;
		color: #777;
		font-family: ui-monospace, SFMono-Regular, Menlo, monospace;
		font-size: 14px;
		word-break: break-all;
	}

	.content {
		flex: 1;
		width: 100%;
		max-width: 760px;
		box-sizing: border-box;
		margin: 0 auto;
		padding: 28px 24px 40px;
	}
	.content p {
		margin: 0 0 16px;
	}

	.grid {
		display: grid;
		grid-template-columns: repeat(auto-fill, minmax(200px, 1fr));
		gap: 16px;
		margin-top: 28px;
	}
	.card {
		overflow: hidden;
		border: 1px solid #e5e5e5;
		border-radius: 10px;
		background: #fff;
	}
	.thumb {
		height: 96px;
	}
	.card h2 {
		margin: 14px 16px 6px;
		color: #111;
		font-size: 17px;
		font-weight: 600;
	}
	.card p {
		margin: 0 16px 16px;
		color: #555;
		font-size: 14px;
		line-height: 1.5;
	}

	.note {
		padding: 16px 24px;
		border-top: 1px solid #e5e5e5;
		color: #999;
		font-size: 12px;
		text-align: center;
	}
</style>
