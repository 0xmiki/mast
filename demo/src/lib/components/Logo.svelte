<script lang="ts">
	let { size = 20 }: { size?: number } = $props();
	const id = 'mast-logo-' + Math.random().toString(36).slice(2, 7);
</script>

<svg width={size} height={size} viewBox="0 0 32 32" aria-label="Mast">
	<defs>
		<linearGradient {id} x1="0" y1="0" x2="1" y2="1">
			<stop offset="0" stop-color="#ff8f5e" />
			<stop offset="1" stop-color="#e2431c" />
		</linearGradient>
	</defs>
	<rect width="32" height="32" rx="8" fill="url(#{id})" />
	<path
		d="M16 6v20M9 11h14M11 11c0 6 2 9 5 10 3-1 5-4 5-10"
		fill="none"
		stroke="#1a0b05"
		stroke-width="2.4"
		stroke-linecap="round"
		stroke-linejoin="round"
	/>
</svg>
