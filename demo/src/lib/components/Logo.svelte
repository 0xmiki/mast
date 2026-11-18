<script lang="ts">
	// The Mast sails as a bare glyph in the current text colour, the way the browser draws them
	// in its toolbar and chips. The path is the mark from browser/tools/brand.mjs.
	let { size = 20 }: { size?: number } = $props();
</script>

<svg width={size} height={size} viewBox="0 0 240 240" fill="currentColor" aria-label="Mast">
	<path d="M90 60V210H20.769A197.115 197.115 0 0 1 90 60ZM120 15V210H210A256.25 256.25 0 0 0 120 15Z" />
</svg>
