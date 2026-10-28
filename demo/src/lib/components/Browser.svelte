<script lang="ts">
	import { onMount } from 'svelte';
	import { browser } from '#lib/mast/browser.svelte.ts';
	import { pacts } from '#lib/mast/pacts.svelte.ts';
	import { attemptEscape } from '#lib/mast/escapes.ts';
	import TabStrip from './chrome/TabStrip.svelte';
	import Toolbar from './chrome/Toolbar.svelte';
	import SidePanel from './chrome/SidePanel.svelte';
	import Content from './Content.svelte';
	import Overlays from './Overlays.svelte';
	import DemoPanel from './DemoPanel.svelte';

	// Budget minutes only tick while a page is actually on screen.
	onMount(() => {
		const t = setInterval(() => {
			if (document.visibilityState === 'visible') pacts.tickUsage(browser.url, 1000);
		}, 1000);
		return () => clearInterval(t);
	});

	function onkeydown(e: KeyboardEvent) {
		const mod = e.ctrlKey || e.metaKey;
		if (!mod) return;
		const k = e.key.toLowerCase();
		if (k === 'l' || k === 'k') browser.omniboxFocus++;
		else if (e.shiftKey && k === 'n') attemptEscape('incognito');
		else if (e.shiftKey && k === 'i') attemptEscape('devtools');
		else if (e.altKey && k === 't') browser.newTab();
		else if (e.altKey && k === 'w') browser.close(browser.activeId);
		else if (e.key === '.') browser.panelOpen = !browser.panelOpen;
		else return;
		e.preventDefault();
	}
</script>

<svelte:window {onkeydown} />

<div class="window">
	<TabStrip />
	<Toolbar />
	<div class="body">
		<Content />
		{#if browser.panelOpen}<SidePanel />{/if}
	</div>
</div>

<Overlays />
<DemoPanel />

<style>
	.window {
		display: flex;
		flex-direction: column;
		height: 100vh;
		background: var(--bg);
	}
	.body {
		display: flex;
		flex: 1;
		min-height: 0;
	}
</style>
