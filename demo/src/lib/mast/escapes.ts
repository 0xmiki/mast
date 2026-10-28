/**
 * Every way a determined user might try to get around a pact, and how Mast answers.
 * Shared by the browser menu and the demo drawer's "try to escape" list.
 */
import { browser, type Dialog } from './browser.svelte';
import { pacts } from './pacts.svelte';
import { formatDate } from './engine';
import { hostOf } from './url';

export type EscapeKind =
	| 'incognito'
	| 'extensions'
	| 'devtools'
	| 'clock'
	| 'clear'
	| 'profile'
	| 'uninstall'
	| 'other-browser';

export const ESCAPES: { kind: EscapeKind; label: string }[] = [
	{ kind: 'incognito', label: 'Open an incognito window' },
	{ kind: 'extensions', label: 'Disable it from the extensions page' },
	{ kind: 'devtools', label: 'Delete the block screen with DevTools' },
	{ kind: 'clock', label: 'Turn the system clock forward' },
	{ kind: 'clear', label: 'Clear all browsing data' },
	{ kind: 'profile', label: 'Make a fresh profile' },
	{ kind: 'uninstall', label: 'Uninstall the browser' },
	{ kind: 'other-browser', label: 'Open a different browser' }
];

function sealedUntil() {
	const end = Math.max(...pacts.live.map((p) => p.endsAt));
	return `Sealed until ${formatDate(end)} by ${pacts.live.length} pact${pacts.live.length === 1 ? '' : 's'}.`;
}

const ANSWERS: Record<EscapeKind, Dialog> = {
	incognito: {
		icon: 'lock',
		title: 'Private windows are sealed',
		body: 'While a pact is in force, incognito and guest windows are turned off. Pacts apply to every window and every profile, so there is no unwatched room to slip into.'
	},
	extensions: {
		icon: 'puzzle',
		title: 'There’s nothing to switch off',
		body: 'Mast’s commitments aren’t an extension. They live in the browser itself, the way Safe Browsing does. The extensions page can’t see them, and no extension can block mast:// pages.'
	},
	devtools: {
		icon: 'code',
		title: 'DevTools are off on sealed sites',
		body: 'The block screen isn’t part of the web page, so there is nothing in the DOM to delete. DevTools stay disabled on sealed sites and work normally everywhere else.'
	},
	clock: {
		icon: 'clock',
		title: 'Clock change ignored',
		body: 'Mast keeps its own time from a monotonic clock checked against network time. Moving the system clock doesn’t move a pact, and the jump is written to your ledger.'
	},
	clear: {
		icon: 'trash',
		title: 'Your pacts survive this',
		body: 'Clearing history, cookies and cache works as usual. Pacts, their amendments and your ledger live in a signed store outside the profile, so they stay.'
	},
	profile: {
		icon: 'shield',
		title: 'Pacts cover every profile',
		body: 'New profiles start with the same live pacts. Adding profiles is sealed while a pact runs, so you can’t start over with a clean slate.'
	},
	uninstall: {
		icon: 'lock',
		title: 'Uninstall is held',
		body: 'While a pact is in force, the Mast system service declines to uninstall. It lets go on its own when the last pact ends.',
		note: 'Platform-specific. The Chromium fork ships this as an OS service.'
	},
	'other-browser': {
		icon: 'door',
		title: 'The one door Mast can’t close alone',
		body: 'A browser can’t stop you from opening a different browser. The plan is Mast Guard, an optional OS companion that closes other browsers while a pact is in force. Without it, Mast relies on the friction of switching.',
		tag: 'Open question'
	}
};

export function attemptEscape(kind: EscapeKind) {
	if (kind === 'devtools' && !pacts.verdict(browser.url).kind.includes('block') && pacts.locked) {
		const host = hostOf(browser.url) || 'this page';
		browser.toast(`DevTools would open on ${host} — it isn’t sealed.`, 'info');
		return;
	}
	if (!pacts.locked && kind !== 'other-browser') {
		browser.toast('No pacts are in force, so this is allowed right now.', 'info');
		return;
	}
	const answer = ANSWERS[kind];
	browser.dialog = { ...answer, note: answer.note ?? (pacts.locked ? sealedUntil() : undefined) };
}
