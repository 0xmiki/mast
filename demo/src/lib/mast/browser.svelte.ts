/** Tabs, navigation and transient UI (panel, menu, toasts, dialogs). */
import { pacts } from './pacts.svelte';
import { clock } from './clock.svelte';
import { normalizeInput } from './url';
import type { Draft } from './types';

export interface Tab {
	id: string;
	history: string[];
	index: number;
}

export interface Toast {
	id: number;
	text: string;
	tone: 'info' | 'lock' | 'good';
}

export interface Dialog {
	icon: 'lock' | 'shield' | 'clock' | 'door' | 'info' | 'puzzle' | 'code' | 'trash';
	title: string;
	body: string;
	note?: string;
	tag?: string;
}

interface HistoryEntry {
	href: string;
	at: number;
}

const KEY = 'mast.tabs';
let nextToast = 1;

function uid() {
	return Math.random().toString(36).slice(2, 10);
}

class Browser {
	tabs = $state<Tab[]>([]);
	activeId = $state('');
	panelOpen = $state(false);
	toasts = $state<Toast[]>([]);
	dialog = $state<Dialog | null>(null);
	history = $state<HistoryEntry[]>([]);
	/** Bumped to ask the omnibox to focus itself. */
	omniboxFocus = $state(0);
	/** Prefill for the next pact wizard (e.g. from the omnibox shield). */
	draftSeed = $state<Partial<Draft> | null>(null);
	/** Bumped on reload so the content re-mounts. */
	reloads = $state(0);

	constructor() {
		if (typeof localStorage !== 'undefined') {
			try {
				const saved = JSON.parse(localStorage.getItem(KEY) ?? 'null');
				if (saved?.tabs?.length) {
					this.tabs = saved.tabs;
					this.activeId = saved.activeId;
					this.history = saved.history ?? [];
				}
			} catch {
				/* ignore */
			}
		}
		if (!this.tabs.length) this.newTab();
		if (!this.active) this.activeId = this.tabs[0].id;
	}

	#save() {
		localStorage.setItem(
			KEY,
			JSON.stringify($state.snapshot({ tabs: this.tabs, activeId: this.activeId, history: this.history.slice(-200) }))
		);
	}

	get active(): Tab {
		return this.tabs.find((t) => t.id === this.activeId) ?? this.tabs[0];
	}

	urlOf(tab: Tab) {
		return tab.history[tab.index];
	}

	get url() {
		return this.urlOf(this.active);
	}

	newTab(href = 'mast://newtab', activate = true) {
		const tab: Tab = { id: uid(), history: [href], index: 0 };
		const at = this.tabs.findIndex((t) => t.id === this.activeId);
		this.tabs.splice(at < 0 ? this.tabs.length : at + 1, 0, tab);
		if (activate) this.activeId = tab.id;
		if (href !== 'mast://newtab') pacts.recordAttempt(href);
		if (activate && href === 'mast://newtab') this.omniboxFocus++;
		this.#save();
		return tab;
	}

	close(id: string) {
		const i = this.tabs.findIndex((t) => t.id === id);
		if (i < 0) return;
		this.tabs.splice(i, 1);
		if (!this.tabs.length) this.newTab();
		else if (this.activeId === id) this.activeId = this.tabs[Math.min(i, this.tabs.length - 1)].id;
		this.#save();
	}

	activate(id: string) {
		this.activeId = id;
		this.#save();
	}

	navigate(input: string, tab = this.active) {
		const href = normalizeInput(input);
		tab.history = [...tab.history.slice(0, tab.index + 1), href];
		tab.index = tab.history.length - 1;
		this.history.push({ href, at: clock.now });
		pacts.recordAttempt(href);
		this.#save();
	}

	back() {
		if (this.active.index > 0) this.active.index--;
		this.#save();
	}

	forward() {
		if (this.active.index < this.active.history.length - 1) this.active.index++;
		this.#save();
	}

	reload() {
		this.reloads++;
		pacts.recordAttempt(this.url);
	}

	/** Leave a blocked page: go back if possible, otherwise to a new tab page. */
	leave() {
		if (this.active.index > 0) this.back();
		else this.navigate('mast://newtab');
	}

	toast(text: string, tone: Toast['tone'] = 'info') {
		const id = nextToast++;
		this.toasts.push({ id, text, tone });
		setTimeout(() => (this.toasts = this.toasts.filter((t) => t.id !== id)), 3600);
	}

	newPact(seed: Partial<Draft> | null = null) {
		this.draftSeed = seed;
		if (this.url.startsWith('mast://pacts/new')) this.reloads++;
		else this.navigate('mast://pacts/new');
	}

	clearHistory() {
		this.history = [];
		this.#save();
	}

	resetTabs() {
		this.tabs = [];
		this.history = [];
		this.newTab();
	}
}

export const browser = new Browser();
