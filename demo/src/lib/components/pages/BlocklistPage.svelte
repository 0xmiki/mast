<script lang="ts">
	import BlockPanel from '../BlockPanel.svelte';
</script>

<div class="page">
	<h1>Blocklist</h1>
	<p class="sub">Pick what to block and for how long. Blocks only get longer. Forever is forever.</p>
	<BlockPanel />
</div>

<style>
	.page {
		display: grid;
		justify-items: center;
		align-content: start;
		min-height: 100%;
		padding: 40px 24px 64px;
		background: var(--bg);
	}
	h1,
	.sub {
		width: min(820px, 100%);
	}
	h1 {
		margin: 0;
		font-size: 28px;
		letter-spacing: -0.02em;
	}
	.sub {
		margin: 6px 0 28px;
		color: var(--muted);
	}
</style>
