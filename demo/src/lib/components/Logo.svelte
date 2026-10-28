<script lang="ts">
	let { size = 20 }: { size?: number } = $props();
	const id = 'mast-logo-' + Math.random().toString(36).slice(2, 7);
</script>

<svg width={size} height={size} viewBox="0 0 32 32" aria-label="Mast">
	<defs>
		<linearGradient {id} x1="0" y1="0" x2="1" y2="1">
			<stop offset="0" stop-color="#ff6a3d" />
			<stop offset="1" stop-color="#c2300f" />
		</linearGradient>
	</defs>
	<rect width="32" height="32" rx="8" fill="url(#{id})" />
	<path d="M14 10.5V24.5H6.6Q7.4 16 14 10.5Z" fill="#ffffff" stroke="#ffffff" stroke-width="1.2" stroke-linejoin="round" />
	<path d="M17 6V24.5H25.4Q24.7 13 17 6Z" fill="#ffffff" stroke="#ffffff" stroke-width="1.2" stroke-linejoin="round" />
</svg>
