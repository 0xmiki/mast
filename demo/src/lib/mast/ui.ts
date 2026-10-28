import type { PactKind, PactStatus } from './types';

/** Accent colour per pact type (CSS colour). */
export const KIND_COLOR: Record<PactKind, string> = {
	block: 'var(--ember)',
	budget: 'var(--warn)',
	focus: 'var(--focus)',
	friction: '#4cc9d9',
	clean: 'var(--good)'
};

export const STATUS_CHIP: Record<PactStatus, { label: string; tone: string }> = {
	grace: { label: 'Regret window', tone: 'warn' },
	upcoming: { label: 'Starts soon', tone: '' },
	active: { label: 'Sealed', tone: 'ember' },
	releasing: { label: 'Releasing', tone: 'warn' },
	released: { label: 'Broken', tone: '' },
	kept: { label: 'Kept', tone: 'good' }
};

export const SUGGESTED_SITES = [
	'youtube.com',
	'x.com',
	'reddit.com',
	'instagram.com',
	'tiktok.com',
	'news.ycombinator.com',
	'facebook.com',
	'netflix.com',
	'twitch.tv',
	'linkedin.com'
];

export const WORK_SITES = [
	'github.com',
	'developer.mozilla.org',
	'en.wikipedia.org',
	'docs.python.org',
	'stackoverflow.com',
	'kit.svelte.dev'
];
