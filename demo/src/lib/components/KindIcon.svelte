<script lang="ts">
	import { Ban, Hourglass, Crosshair, Wind, Sparkles } from '@lucide/svelte';
	import type { PactKind } from '#lib/mast/types.ts';

	let { kind, size = 16 }: { kind: PactKind; size?: number } = $props();
	const icons = { block: Ban, budget: Hourglass, focus: Crosshair, friction: Wind, clean: Sparkles };
	const Icon = $derived(icons[kind]);
</script>

<Icon {size} strokeWidth={2} />
