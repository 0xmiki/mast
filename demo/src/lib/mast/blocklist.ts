/**
 * Known distractions, grouped. Users can add their own under "Your sites".
 * The data lives in /blocklist/blocklist.json at the repo root, shared with the Chromium build.
 */
import data from '../../../../blocklist/blocklist.json';
import { matches } from './url';

export interface Service {
	name: string;
	domains: string[];
	/** Subdomains left open, e.g. aws.amazon.com under amazon.com. */
	except?: string[];
}

export const CATEGORIES: { id: string; name: string; services: Service[] }[] = data.categories;

export const PRESETS = CATEGORIES.map((c) => ({ group: c.name, sites: c.services.flatMap((s) => s.domains) }));

export const PRESET_SITES = PRESETS.flatMap((g) => g.sites);

const EXCEPT = CATEGORIES.flatMap((c) => c.services.flatMap((s) => s.except ?? []));

/** Hosts a block never covers, because work depends on them. */
export function isException(host: string) {
	return EXCEPT.some((e) => matches(host, e));
}
