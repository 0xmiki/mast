<script lang="ts">
	import { browser } from '#lib/mast/browser.svelte.ts';
	import { focus } from '#lib/mast/focus.svelte.ts';
	import { formatDuration } from '#lib/mast/engine.ts';
	import { attemptEscape } from '#lib/mast/escapes.ts';
	import TabStrip from './chrome/TabStrip.svelte';
	import Toolbar from './chrome/Toolbar.svelte';
	import Content from './Content.svelte';
	import Overlays from './Overlays.svelte';
	import DemoPanel from './DemoPanel.svelte';

	// Celebrate when a session runs out on its own.
	let running: { id: string; startedAt: number; endsAt: number } | null = null;
	$effect(() => {
		const s = focus.active;
		if (s) running = { id: s.id, startedAt: s.startedAt, endsAt: s.endsAt };
		else if (running) {
			if (focus.sessions.some((x) => x.id === running!.id)) {
				browser.toast(`Session done: ${formatDuration(running.endsAt - running.startedAt)} of deep work.`, 'good', undefined, 6000);
			}
			running = null;
		}
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
