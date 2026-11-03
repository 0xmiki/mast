#!/usr/bin/env node
/**
 * Checks blocklist.json: shape, duplicates, exceptions, and that every domain resolves.
 *   node blocklist/check.mjs            # full check, needs network
 *   node blocklist/check.mjs --offline  # shape only
 */
import fs from 'node:fs';
import dns from 'node:dns/promises';

const file = new URL('./blocklist.json', import.meta.url);
const list = JSON.parse(fs.readFileSync(file, 'utf8'));
const offline = process.argv.includes('--offline');
const problems = [];
const HOST = /^(?=.{1,253}$)([a-z0-9](?:[a-z0-9-]{0,61}[a-z0-9])?\.)+[a-z0-9-]{2,63}$/;

const seen = new Map();
const ids = new Set();
let services = 0;
for (const c of list.categories) {
	if (ids.has(c.id)) problems.push(`duplicate category id ${c.id}`);
	ids.add(c.id);
	const names = new Set();
	for (const s of c.services) {
		services++;
		if (names.has(s.name)) problems.push(`${c.id}: duplicate service ${s.name}`);
		names.add(s.name);
		if (!s.domains?.length) problems.push(`${c.id}/${s.name}: no domains`);
		for (const d of s.domains ?? []) {
			if (!HOST.test(d)) problems.push(`${c.id}/${s.name}: bad host "${d}"`);
			if (seen.has(d)) problems.push(`${d} is in both ${seen.get(d)} and ${c.id}/${s.name}`);
			seen.set(d, `${c.id}/${s.name}`);
		}
		for (const e of s.except ?? []) {
			if (!s.domains.some((d) => e.endsWith('.' + d))) problems.push(`${c.id}/${s.name}: exception ${e} is not under any of its domains`);
		}
	}
}

// A domain under another listed domain is redundant (matching already covers subdomains).
const all = [...seen.keys()];
for (const d of all) {
	const parent = all.find((p) => p !== d && d.endsWith('.' + p));
	if (parent) problems.push(`${d} is already covered by ${parent} (${seen.get(parent)})`);
}

if (!offline) {
	const dead = [];
	const queue = [...all];
	const resolve = async (d) => {
		for (let i = 0; i < 2; i++) {
			try {
				await Promise.race([dns.lookup(d), new Promise((_, no) => setTimeout(() => no(new Error('timeout')), 5000))]);
				return true;
			} catch {}
		}
		return false;
	};
	await Promise.all(
		Array.from({ length: 24 }, async () => {
			for (let d; (d = queue.shift()); ) if (!(await resolve(d))) dead.push(d);
		})
	);
	for (const d of dead.sort()) problems.push(`${d} does not resolve (${seen.get(d)})`);
}

console.log(`${list.categories.length} categories, ${services} services, ${all.length} domains`);
if (problems.length) {
	console.log(problems.map((p) => '  ✗ ' + p).join('\n'));
	process.exit(1);
}
console.log('  ✓ all good');
