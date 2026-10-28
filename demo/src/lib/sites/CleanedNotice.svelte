<script lang="ts">
	let { label, dark = false }: { label: string; dark?: boolean } = $props();
</script>

<div class="cleaned" class:dark>
	<svg viewBox="0 0 24 24" width="14" height="14" aria-hidden="true">
		<path
			d="M12 2 4 5v6c0 5 3.4 9.4 8 11 4.6-1.6 8-6 8-11V5l-8-3Z"
			fill="none"
			stroke="currentColor"
			stroke-width="2"
			stroke-linejoin="round"
		/>
	</svg>
	<span>Hidden by Mast · {label}</span>
</div>

<style>
	.cleaned {
		display: flex;
		align-items: center;
		justify-content: center;
		gap: 8px;
		padding: 14px 16px;
		border: 1px dashed rgb(0 0 0 / 0.18);
		border-radius: 12px;
		color: rgb(0 0 0 / 0.5);
		font: 500 13px/1.3 var(--font);
		background: rgb(0 0 0 / 0.02);
	}
	.cleaned.dark {
		border-color: rgb(255 255 255 / 0.18);
		color: rgb(255 255 255 / 0.55);
		background: rgb(255 255 255 / 0.03);
	}
</style>
