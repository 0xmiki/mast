/**
 * Every way a determined user might try to get around a session, and how Mast answers.
 * Shared by the browser menu and the demo drawer's "try to escape" list.
 */
import { browser, type Dialog } from './browser.svelte';
import { focus } from './focus.svelte';
import { formatWhen } from './engine';
import { clock } from './clock.svelte';
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

function blockedUntil() {
	return `Blocked until ${formatWhen(focus.active!.endsAt, clock.now)}.`;
}

const ANSWERS: Record<EscapeKind, Dialog> = {
	incognito: {
		icon: 'lock',
		title: 'Private windows are off',
		body: 'During a session, incognito and guest windows are turned off. Blocking covers every window and every profile, so there is no unwatched room to slip into.'
	},
	extensions: {
		icon: 'puzzle',
		title: 'There’s nothing to switch off',
		body: 'Blocking isn’t an extension. It lives in the browser itself, the way Safe Browsing does. The extensions page can’t see it, and no extension can get around it.'
	},
	devtools: {
		icon: 'code',
		title: 'DevTools are off on blocked sites',
		body: 'The block screen isn’t part of the web page, so there is nothing in the DOM to delete. DevTools stay off on blocked sites and work normally everywhere else.'
	},
	clock: {
		icon: 'clock',
		title: 'Clock change ignored',
		body: 'Mast keeps its own time from a monotonic clock checked against network time. Moving the system clock doesn’t end a session early.'
	},
	clear: {
		icon: 'trash',
		title: 'The session survives this',
		body: 'Clearing history, cookies and cache works as usual. The running session and your blocklist live in a signed store outside the profile, so they stay.'
	},
	profile: {
		icon: 'shield',
		title: 'Sessions cover every profile',
		body: 'Adding or switching profiles is off during a session, so there is no clean slate to escape into.'
	},
	uninstall: {
		icon: 'lock',
		title: 'Uninstall is held',
		body: 'During a session, the Mast system service declines to uninstall. It lets go on its own when the session ends.',
		note: 'Platform-specific. The Chromium fork ships this as an OS service.'
	},
	'other-browser': {
		icon: 'door',
		title: 'The one door Mast can’t close alone',
		body: 'A browser can’t stop you from opening a different browser. The plan is Mast Guard, an optional OS companion that closes other browsers during a session. Without it, Mast relies on the friction of switching.',
		tag: 'Open question'
	}
};

export function attemptEscape(kind: EscapeKind) {
	if (kind === 'devtools' && focus.active && !focus.blocks(browser.url)) {
		const host = hostOf(browser.url) || 'this page';
		browser.toast(`DevTools would open on ${host}. It isn’t blocked.`, 'info');
		return;
	}
	if (!focus.active && kind !== 'other-browser') {
		browser.toast('No session running, so this is allowed right now.', 'info');
		return;
	}
	const answer = ANSWERS[kind];
	browser.dialog = { ...answer, note: answer.note ?? (focus.active ? blockedUntil() : undefined) };
}
