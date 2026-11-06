#!/usr/bin/env node
// Reproduce files of the builder's base tree locally, without a checkout:
// Chromium at chromium_version.txt + ungoogled-chromium's patches (series order, minus
// patches/ungoogled-drop.list, plus patches/ungoogled-extra.list), for just these paths.
// Domain substitution is not applied (keep anchors away from URLs).
//
//   node tools/base-file.mjs chrome/browser/prefs/browser_prefs.cc ...
//   -> prints the local path of each file (cached under ~/.cache/mast/base/<version>/)
//
// Used by `./mast check-patches` to validate make-patches.py anchors before renting the server.
import { execFileSync } from 'node:child_process';
import fs from 'node:fs';
import os from 'node:os';
import path from 'node:path';

const here = path.dirname(new URL(import.meta.url).pathname);
const root = path.resolve(here, '..');
const version = fs.readFileSync(path.join(root, 'chromium_version.txt'), 'utf8').trim();
const ugTag = fs.readFileSync(path.join(root, 'ungoogled_version.txt'), 'utf8').trim();
const cache = path.join(os.homedir(), '.cache/mast');
const baseDir = path.join(cache, 'base', `${version}+${ugTag}`);
const ugDir = path.join(cache, 'ungoogled', ugTag);

const list = (f) => fs.readFileSync(path.join(root, 'patches', f), 'utf8')
  .split('\n').map((l) => l.replace(/#.*/, '').trim()).filter(Boolean);

async function get(url) {
  for (let i = 0; ; i++) {
    try {
      const r = await fetch(url, { signal: AbortSignal.timeout(30000) });
      if (!r.ok) throw new Error(`${r.status} ${url}`);
      return r;
    } catch (e) {
      if (i >= 3) throw e;
      await new Promise((res) => setTimeout(res, 1000 * (i + 1)));
    }
  }
}

// Run fn over items, n at a time.
async function pool(items, n, fn) {
  const q = [...items];
  await Promise.all(Array.from({ length: n }, async () => { while (q.length) await fn(q.shift()); }));
}

async function ungoogledSeries() {
  const seriesFile = path.join(ugDir, 'series');
  if (!fs.existsSync(seriesFile)) {
    fs.mkdirSync(ugDir, { recursive: true });
    const raw = `https://raw.githubusercontent.com/ungoogled-software/ungoogled-chromium/${ugTag}/patches/`;
    const series = (await (await get(raw + 'series')).text());
    const names = series.split('\n').map((l) => l.replace(/#.*/, '').trim()).filter(Boolean);
    await pool(names, 8, async (n) => {
      const out = path.join(ugDir, 'p', n);
      fs.mkdirSync(path.dirname(out), { recursive: true });
      fs.writeFileSync(out, await (await get(raw + n)).text());
    });
    fs.writeFileSync(seriesFile, series);
  }
  const drop = new Set(list('ungoogled-drop.list').map((n) => path.basename(n)));
  const extra = new Set(list('ungoogled-extra.list').map((n) => path.basename(n)));
  return fs.readFileSync(seriesFile, 'utf8').split('\n').map((l) => l.replace(/#.*/, '').trim())
    .filter(Boolean)
    .filter((n) => !drop.has(path.basename(n)))
    .filter((n) => !n.startsWith('extra/') || extra.has(path.basename(n)));
}

// The part of a unified diff that changes `file`.
function sectionFor(patchText, file) {
  const parts = patchText.split(/^(?=diff |--- a\/)/m);
  return parts.filter((p) => new RegExp(`^\\+\\+\\+ b/${file.replace(/[.*+?^${}()|[\]\\]/g, '\\$&')}$`, 'm').test(p)).join('');
}

async function baseFile(file) {
  const out = path.join(baseDir, file);
  if (fs.existsSync(out)) return out;
  const url = `https://chromium.googlesource.com/chromium/src/+/refs/tags/${version}/${file}?format=TEXT`;
  const text = Buffer.from(await (await get(url)).text(), 'base64').toString('utf8');
  fs.mkdirSync(path.dirname(out), { recursive: true });
  fs.writeFileSync(out, text);
  for (const name of await ungoogledSeries()) {
    const section = sectionFor(fs.readFileSync(path.join(ugDir, 'p', name), 'utf8'), file);
    if (!section) continue;
    execFileSync('patch', ['--silent', '--forward', '-p1', '-d', baseDir], { input: section });
    process.stderr.write(`  ${file}: applied ${name}\n`);
  }
  return out;
}

for (const f of process.argv.slice(2)) console.log(await baseFile(f));
