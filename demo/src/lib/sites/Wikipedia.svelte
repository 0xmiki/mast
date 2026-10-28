<script lang="ts">
	import type { SiteProps } from './types';

	let { url, navigate }: SiteProps = $props();

	function decode(s: string): string {
		try {
			return decodeURIComponent(s);
		} catch {
			return s;
		}
	}

	/** Page title with underscores, e.g. "Ulysses_pact". "/" shows the main page. */
	const slug = $derived(url.pathname.startsWith('/wiki/') ? decode(url.pathname.slice(6)) : 'Main_Page');
	const title = $derived(slug.replace(/_/g, ' '));
	const isMain = $derived(slug === 'Main_Page');
	const isUlysses = $derived(slug === 'Ulysses_pact');

	function onSearch(e: Event) {
		e.preventDefault();
		const q = String(new FormData(e.currentTarget as HTMLFormElement).get('q') ?? '').trim();
		if (q) navigate('/wiki/' + encodeURIComponent(q.replace(/ /g, '_')));
	}
</script>

<div class="wp">
	<header class="head">
		<a class="word" href="/">
			Wikipedia
			<span class="tag">The Free Encyclopedia</span>
		</a>
		<form class="search" role="search" onsubmit={onSearch}>
			<input name="q" type="search" placeholder="Search Wikipedia" aria-label="Search Wikipedia" autocomplete="off" />
			<button type="submit">Search</button>
		</form>
	</header>

	<div class="shell">
		<aside class="side">
			<h3>Navigation</h3>
			<ul>
				<li><a href="/">Main page</a></li>
				<li><a href="/wiki/Portal:Current_events">Current events</a></li>
				<li><a href="/wiki/Special:Random">Random article</a></li>
				<li><a href="/wiki/Wikipedia:About">About Wikipedia</a></li>
				<li><a href="/wiki/Wikipedia:Contact_us">Contact us</a></li>
			</ul>
			<h3>Contribute</h3>
			<ul>
				<li><a href="/wiki/Help:Contents">Help</a></li>
				<li><a href="/wiki/Help:Introduction">Learn to edit</a></li>
				<li><a href="/wiki/Wikipedia:Community_portal">Community portal</a></li>
			</ul>
		</aside>

		<main class="main">
			<div class="tabs">
				<span class="tab on">{isMain ? 'Main Page' : 'Article'}</span>
				{#if !isMain}<span class="tab">Talk</span>{/if}
				<span class="right">Read &middot; Edit &middot; View history</span>
			</div>

			{#if isMain}
				<h1>Main Page</h1>
				<div class="welcome">
					<h2>Welcome to Wikipedia</h2>
					<p>
						The Free Encyclopedia that anyone can edit. Millions of articles in hundreds of languages,
						written and maintained by volunteers.
					</p>
					<p><a href="/wiki/Special:Random">Random article</a> &middot; <a href="/wiki/Help:Contents">Help</a></p>
				</div>
				<div class="feat">
					<h3>From today's featured article</h3>
					<p class="ft"><a href="/wiki/Ulysses_pact">Ulysses pact</a></p>
					<p>
						A Ulysses pact is a form of precommitment, named after the story in which Odysseus has
						himself tied to the mast so that he can hear the Sirens safely.
					</p>
					<p><a href="/wiki/Ulysses_pact">Read the full article</a></p>
				</div>
			{:else if isUlysses}
				<h1>Ulysses pact</h1>
				<p>
					<b>Ulysses pact</b> (also <i>Ulysses contract</i>) is a form of
					<a href="/wiki/Precommitment">precommitment</a> in which a person deliberately binds themselves to a course
					of action, or removes an option, so that a later and weaker version of themselves cannot choose otherwise.
					The name comes from the story of <a href="/wiki/Odysseus">Odysseus</a>, who had his crew tie him to the mast
					so that he could hear the song of the <a href="/wiki/Sirens">Sirens</a> without steering his ship onto the
					rocks.<sup class="cite">[1]</sup>
				</p>

				<div class="toc">
					<div class="toc-h">Contents</div>
					<ol>
						<li>Origins in the Odyssey</li>
						<li>Self-control and the planner</li>
						<li>Examples</li>
						<li>See also</li>
						<li>References</li>
					</ol>
				</div>

				<h2>Origins in the Odyssey</h2>
				<p>
					In Book 12 of Homer's <a href="/wiki/Odyssey">Odyssey</a>, the goddess Circe warns Odysseus that the Sirens sing
					in a voice that lures sailors onto the rocks. She advises him to stop his crew's ears with beeswax, and to have
					himself bound to the mast with orders not to be released, however much he begs. Odysseus hears the song and
					survives. The episode became the standard image of a commitment made in advance against a foreseeable temptation.
				</p>
				<p>
					The philosopher Jon Elster used the episode as the title of <i>Ulysses and the Sirens</i> (1979), a study of
					rationality and self-restraint.<sup class="cite">[2]</sup> Elster argued that binding oneself is not a failure of
					reason but one of its tools, provided the binding is done while the situation is still seen clearly.
				</p>

				<h2>Self-control and the planner</h2>
				<p>
					Economists have long modelled people whose preferences change over time. In their 1981 paper
					"An Economic Theory of Self-Control", Richard Thaler and Hersh Shefrin described the individual as a far-sighted
					planner and a short-sighted doer whose interests conflict.<sup class="cite">[3]</sup> The planner can limit the
					doer by removing options, which is the logic of a Ulysses pact. The classical name for acting against one's own
					better judgement is <a href="/wiki/Akrasia">akrasia</a>, or weakness of will, and precommitment is one of the
					oldest responses to it.
				</p>
				<p>
					Thomas Schelling treated the same problem as a contest between a person's present and future selves, in essays
					later collected in <i>Choice and Consequence</i> (1984).<sup class="cite">[4]</sup> His point was that the most
					reliable way to keep a promise to oneself is often to make the wrong choice impossible, rather than merely
					unattractive.
				</p>

				<h2>Examples</h2>
				<h3>Christmas savings clubs</h3>
				<p>
					In the early twentieth century, many American banks offered Christmas Club accounts. Customers made small regular
					deposits through the year, and the money could only be withdrawn at the end of the year, in time for holiday
					shopping. The account turned saving into a rule rather than a decision, and it was popular for exactly that reason.
				</p>
				<h3>Commitment devices</h3>
				<p>
					Modern commitment devices include apps that charge a user's card when they miss a self-set goal, and savings
					accounts or memberships with penalties for early withdrawal. Economists have studied how these products work in
					practice, including in a review of field experiments by Bryan, Karlan and Nelson (2010).<sup class="cite">[5]</sup>
				</p>
				<h3>Medication and recovery</h3>
				<p>
					Disulfiram, sold under the brand name Antabuse, is prescribed to some people in recovery from alcohol dependence.
					It causes an unpleasant reaction if alcohol is consumed, so a patient who takes it has made an advance decision
					that is hard to reverse in the moment. Its effectiveness depends heavily on whether the patient keeps taking it,
					which is itself a commitment problem.
				</p>

				<h2>See also</h2>
				<ul>
					<li><a href="/wiki/Precommitment">Precommitment</a></li>
					<li><a href="/wiki/Sirens">Sirens</a></li>
					<li><a href="/wiki/Odyssey">Odyssey</a></li>
					<li><a href="/wiki/Akrasia">Akrasia</a></li>
					<li><a href="/wiki/Behavioral_economics">Behavioral economics</a></li>
				</ul>

				<h2>References</h2>
				<ol class="refs">
					<li>Homer. <i>Odyssey</i>, Book 12.</li>
					<li>Elster, Jon (1979). <i>Ulysses and the Sirens</i>. Cambridge University Press.</li>
					<li>Thaler, Richard H.; Shefrin, H. M. (1981). "An Economic Theory of Self-Control". <i>Journal of Political Economy</i> 89 (2).</li>
					<li>Schelling, Thomas C. (1984). <i>Choice and Consequence</i>. Harvard University Press.</li>
					<li>Bryan, Gharad; Karlan, Dean; Nelson, Scott (2010). "Commitment Devices". <i>Annual Review of Economics</i> 2.</li>
				</ol>
			{:else}
				<h1>{title}</h1>
				<div class="stub">
					This article is a stub. You can help Wikipedia by expanding it.
				</div>
				<p>
					{title} is a topic that has drawn attention from writers and researchers, although relatively little has been
					written about it in general terms. This article is a short introduction and will be expanded as sources become
					available.
				</p>
				<h2>Background</h2>
				<p>
					The term {title} is used in a number of settings, and its meaning has shifted with the context in which it
					appears. Early uses were mostly descriptive, and later uses have tended to be more formal.
				</p>
				<h2>Usage</h2>
				<p>
					Writers who use {title} generally assume some familiarity with the subject, so introductions are often brief.
					Further detail can be found in the sources listed below once they have been added.
				</p>
				<h2>See also</h2>
				<ul>
					<li><a href="/">Main Page</a></li>
					<li><a href="/wiki/Ulysses_pact">Ulysses pact</a></li>
				</ul>
			{/if}
		</main>
	</div>
</div>

<style>
	.wp { min-height: 100%; background: #fff; color: #202122; font: 14px/1.6 sans-serif; }
	a { color: #3366cc; text-decoration: none; }
	a:hover { text-decoration: underline; }
	.head { display: flex; align-items: center; gap: 24px; padding: 12px 20px; border-bottom: 1px solid #a2a9b1; }
	.word { color: #000; font: 26px/1 'Linux Libertine', 'Georgia', 'Times', serif; }
	.word:hover { text-decoration: none; }
	.tag { display: block; margin-top: 3px; color: #54595d; font: 11px/1.2 sans-serif; letter-spacing: 0.02em; text-transform: uppercase; }
	.search { display: flex; width: min(420px, 50%); margin-left: auto; }
	.search input { flex: 1; min-width: 0; padding: 6px 10px; border: 1px solid #a2a9b1; border-radius: 2px 0 0 2px; font: inherit; font-size: 14px; }
	.search button { padding: 6px 12px; border: 1px solid #a2a9b1; border-left: none; border-radius: 0 2px 2px 0; background: #f8f9fa; font: inherit; font-size: 13px; cursor: pointer; }
	.shell { display: flex; }
	.side { flex: none; width: 170px; padding: 14px 14px 14px 20px; font-size: 12.5px; }
	.side h3 { margin: 14px 0 4px; color: #54595d; font: 600 12px/1.4 sans-serif; }
	.side h3:first-child { margin-top: 0; }
	.side ul { margin: 0; padding: 0; list-style: none; }
	.side li { padding: 2px 0; }
	.main { flex: 1; min-width: 0; max-width: 880px; padding: 14px 24px 48px; }
	.tabs { display: flex; align-items: flex-end; gap: 2px; margin-bottom: 18px; border-bottom: 1px solid #a2a9b1; font-size: 13px; }
	.tab { padding: 5px 12px; border: 1px solid transparent; border-bottom: none; border-radius: 2px 2px 0 0; color: #3366cc; }
	.tab.on { border-color: #a2a9b1; background: #fff; color: #202122; }
	.right { margin-left: auto; padding-bottom: 5px; color: #3366cc; }
	h1 { margin: 0 0 12px; padding-bottom: 4px; border-bottom: 1px solid #a2a9b1; color: #000; font: 400 30px/1.25 'Linux Libertine', 'Georgia', 'Times', serif; }
	h2 { margin: 22px 0 10px; padding-bottom: 2px; border-bottom: 1px solid #a2a9b1; color: #000; font: 400 22px/1.3 'Linux Libertine', 'Georgia', 'Times', serif; }
	h3 { margin: 16px 0 6px; color: #000; font: 700 16px/1.3 'Linux Libertine', 'Georgia', 'Times', serif; }
	p { margin: 0 0 12px; }
	ul, ol { margin: 0 0 12px; padding-left: 24px; }
	.cite { margin-left: 1px; color: #3366cc; font-size: 11px; }
	.toc { display: inline-block; max-width: 260px; margin: 0 0 18px; padding: 10px 16px; border: 1px solid #a2a9b1; background: #f8f9fa; font-size: 13px; }
	.toc-h { margin-bottom: 4px; font-weight: 700; text-align: center; }
	.toc ol { margin: 0; padding-left: 22px; }
	.stub { margin: 0 0 16px; padding: 8px 12px; border: 1px solid #a2a9b1; border-left: 4px solid #36c; background: #f8f9fa; font-size: 13px; }
	.welcome { margin-bottom: 16px; padding: 14px 18px; border: 1px solid #a2a9b1; background: #f8f9fa; }
	.welcome h2 { margin-top: 0; border: none; font-size: 20px; }
	.feat { padding: 14px 18px; border: 1px solid #a2a9b1; }
	.feat h3 { margin-top: 0; font-size: 15px; }
	.ft { font: 700 20px/1.3 'Linux Libertine', 'Georgia', 'Times', serif; }
	.refs { font-size: 13px; }
</style>
