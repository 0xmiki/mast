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
	<path d="M5.5 24 13.5 6.5V24Z" fill="#1a0b05" />
	<path d="M16.5 6.5 26.5 24H16.5Z" fill="#1a0b05" />
</svg>
