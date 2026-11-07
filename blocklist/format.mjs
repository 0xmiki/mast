#!/usr/bin/env node
/** Rewrites blocklist.json in its canonical layout: one service per line, so diffs stay readable. */
import fs from 'node:fs';

const file = new URL('./blocklist.json', import.meta.url);
const list = JSON.parse(fs.readFileSync(file, 'utf8'));
const s = (v) => JSON.stringify(v);
const service = (x) => {
	const parts = [`"name": ${s(x.name)}`, `"domains": ${s(x.domains).replaceAll('","', '", "')}`];
	if (x.except) parts.push(`"except": ${s(x.except).replaceAll('","', '", "')}`);
	return `\t\t\t\t{ ${parts.join(', ')} }`;
};
const out = `{
	"version": ${list.version},
	"updated": ${s(list.updated)},
	"categories": [
${list.categories
	.map((c) => `\t\t{\n\t\t\t"id": ${s(c.id)},\n\t\t\t"name": ${s(c.name)},\n${c.feed ? `\t\t\t"feed": ${s(c.feed)},\n` : ''}\t\t\t"services": [\n${c.services.map(service).join(',\n')}\n\t\t\t]\n\t\t}`)
	.join(',\n')}
	]
}
`;
fs.writeFileSync(file, out);
