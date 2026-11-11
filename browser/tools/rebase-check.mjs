#!/usr/bin/env node
// What Mast depends on in Chromium, and what a new release changes in it. Local and free:
// it reads files through tools/base-file.mjs (Chromium's source site + ungoogled's patches).
//
//   node tools/rebase-check.mjs inventory <work>
//     -> docs/UPSTREAM.md: every upstream file make-patches.py edits (and why), the Chromium
//        headers src/mast includes, the methods it overrides, its upstream GN deps.
//        ./mast check-patches runs this, so the file follows the code.
//   node tools/rebase-check.mjs report <work> <version> <ungoogled tag>
//     -> docs/rebase/<version>.md: what moving to that release breaks or puts at risk.
//        ./mast check-rebase runs this after make-patches.py has run in <work>/from (the
//        pinned release) and <work>/to (the target, MAST_KEEP_GOING=1).
//
// What it can't see: Chromium code that behaves differently, build files we don't depend on
// by name, UI the screenshots rely on (menu order in remote/shots.py). A build still decides.
import { execFileSync, spawnSync } from 'node:child_process';
import fs from 'node:fs';
import os from 'node:os';
import path from 'node:path';

const here = path.dirname(new URL(import.meta.url).pathname);
const root = path.resolve(here, '..');
const read = (f) => fs.readFileSync(f, 'utf8');
const pinned = read(path.join(root, 'chromium_version.txt')).trim();
const pinnedTag = read(path.join(root, 'ungoogled_version.txt')).trim();
const makePatches = read(path.join(root, 'remote/make-patches.py')).split('\n');

// ------------------------------------------------------------------ what Mast uses

function walk(dir, keep) {
  const out = [];
  for (const e of fs.readdirSync(dir, { withFileTypes: true })) {
    const p = path.join(dir, e.name);
    if (e.isDirectory()) out.push(...walk(p, keep));
    else if (keep(p)) out.push(p);
  }
  return out;
}
const srcDir = path.join(root, 'src');
const rel = (p) => path.relative(srcDir, p);
const sources = walk(path.join(srcDir, 'mast'), (p) => /\.(cc|h|mm)$/.test(p));
const stripComments = (t) => t.replace(/\/\*[\s\S]*?\*\//g, '').replace(/\/\/.*$/gm, '');
const code = new Map(sources.map((f) => [f, stripComments(read(f))]));

// Upstream headers, each with the Mast files that include it.
const includes = new Map();
for (const f of sources) {
  for (const [, h] of read(f).matchAll(/^#include "([^"]+)"/gm)) {
    if (h.startsWith('mast/')) continue;
    if (!includes.has(h)) includes.set(h, new Set());
    includes.get(h).add(rel(f));
  }
}
// Names Mast's code uses from upstream: methods it overrides, methods it calls, constants.
const overrides = new Set(), calls = new Set(), constants = new Set();
for (const t of code.values()) {
  for (const [, m] of t.matchAll(/\b([A-Z]\w+)\s*\([^;{}]*\)\s*(?:const\s*)?(?:override|final)\b/g)) overrides.add(m);
  for (const [, m] of t.matchAll(/(?:\.|->|::)([A-Z]\w+)\s*\(/g)) calls.add(m);
  for (const [, m] of t.matchAll(/\b(k[A-Z]\w+)\b/g)) constants.add(m);
}
// Names Mast defines itself never come from upstream.
const ours = new Set();
for (const t of code.values()) {
  for (const [, m] of t.matchAll(/\b(?:class|struct|enum(?: class)?)\s+([A-Z]\w+)/g)) ours.add(m);
  for (const [, m] of t.matchAll(/^(?:constexpr|const)\s[^=(]*\b(k[A-Z]\w+)\b/gm)) ours.add(m);
}
for (const s of [calls, constants]) for (const n of ours) s.delete(n);

// GN deps outside //mast, each with the BUILD.gn files that name them.
const gnDeps = new Map();
for (const f of walk(path.join(srcDir, 'mast'), (p) => p.endsWith('BUILD.gn'))) {
  for (const [, d] of read(f).matchAll(/"(\/\/[^":]+(?::[^"]+)?)"/g)) {
    if (d.startsWith('//mast')) continue;
    if (!gnDeps.has(d)) gnDeps.set(d, new Set());
    gnDeps.get(d).add(rel(f));
  }
}
const gnTarget = (d) => {
  const [dir, name] = d.slice(2).split(':');
  return { file: `${dir}/BUILD.gn`, name: name ?? path.basename(dir) };
};

// The comment above an edit in make-patches.py: why it's there. One comment often covers a
// run of edits (say an edit and its BUILD.gn dep), so walk up through the run to it.
function why(line) {
  const out = [];
  let i = line - 2;
  while (i >= 0 && makePatches[i].trim() && !/^#/.test(makePatches[i])) i--;
  for (; i >= 0 && /^#/.test(makePatches[i]); i--) {
    if (/^# -{5,}/.test(makePatches[i])) break;
    out.unshift(makePatches[i].replace(/^\s*#\s?/, ''));
  }
  return out.join(' ').trim();
}
function section(line) {
  for (let i = line - 1; i >= 0; i--) {
    const m = makePatches[i].match(/^# -{5,} (.+)$/);
    if (m) return m[1].trim();
  }
  return '';
}

// ------------------------------------------------------------------ base files

// Local paths of `files` at a release (null where Chromium's source has no such file).
function baseFiles(version, tag, files) {
  const out = new Map();
  for (let i = 0; i < files.length; i += 40) {
    const chunk = files.slice(i, i + 40);
    const r = spawnSync('node', [path.join(here, 'base-file.mjs'), ...chunk], {
      encoding: 'utf8', maxBuffer: 1 << 26,
      env: { ...process.env, MAST_CHROMIUM_VERSION: version, MAST_UNGOOGLED_TAG: tag, MAST_ALLOW_MISSING: '1' },
    });
    if (r.status !== 0) throw new Error(`base-file.mjs failed for ${version}:\n${r.stderr}`);
    r.stdout.trim().split('\n').forEach((p, j) => out.set(chunk[j], p === '-' ? null : p));
  }
  return out;
}
function ungoogled(version, tag) {
  const env = { ...process.env, MAST_CHROMIUM_VERSION: version, MAST_UNGOOGLED_TAG: tag };
  const applied = execFileSync('node', [path.join(here, 'base-file.mjs'), '--series'], { encoding: 'utf8', env })
    .trim().split('\n').filter(Boolean);
  const dir = path.join(os.homedir(), '.cache/mast/ungoogled', tag);
  const all = read(path.join(dir, 'series')).split('\n').map((l) => l.replace(/#.*/, '').trim()).filter(Boolean);
  const touches = (name) => {
    const p = path.join(dir, 'p', name);
    return fs.existsSync(p) ? [...read(p).matchAll(/^\+\+\+ b\/(\S+)/gm)].map((m) => m[1]) : [];
  };
  return { applied, all, touches };
}

// Changed regions of a file between two releases: old-side line range and the diff lines.
function hunks(a, b) {
  const r = spawnSync('diff', ['-U0', a, b], { encoding: 'utf8', maxBuffer: 1 << 26 });
  return r.stdout.split(/^(?=@@ )/m).slice(1).map((h) => {
    const [, s, n] = h.match(/^@@ -(\d+)(?:,(\d+))?/);
    const range = [Number(s), Number(s) + Math.max(Number(n ?? 1), 1) - 1];
    range.text = h.split('\n').slice(1).filter(Boolean);
    return range;
  });
}
// Our edits per file, as old-side line ranges, from generated patches.
function ourHunks(dir) {
  const out = new Map();
  for (const f of fs.readdirSync(dir).filter((f) => f.endsWith('.patch'))) {
    let file = null;
    for (const line of read(path.join(dir, f)).split('\n')) {
      const m = line.match(/^\+\+\+ b\/(\S+)/);
      if (m) { file = m[1]; if (!out.has(file)) out.set(file, []); continue; }
      const h = line.match(/^@@ -(\d+)(?:,(\d+))? /);
      if (h && file) out.get(file).push([Number(h[1]), Number(h[1]) + Math.max(Number(h[2] ?? 1), 1) - 1]);
    }
  }
  return out;
}
const near = (x, y, margin = 12) => x[0] - margin <= y[1] && y[0] - margin <= x[1];

const md = (s) => String(s).replace(/\|/g, '\\|');
const code1 = (s) => '`' + String(s).replace(/`/g, "'") + '`';

// ------------------------------------------------------------------ inventory

function inventory(work) {
  const edits = JSON.parse(read(path.join(work, 'edits.json')));
  const byPatch = new Map();
  for (const e of edits) {
    if (!byPatch.has(e.patch)) byPatch.set(e.patch, []);
    byPatch.get(e.patch).push(e);
  }
  const files = new Set(edits.map((e) => e.path));
  const list = (f) => read(path.join(root, 'patches', f)).split('\n').map((l) => l.replace(/#.*/, '').trim()).filter(Boolean);
  const L = [];
  L.push('# Where Mast touches Chromium', '');
  L.push(`Generated by \`./mast check-patches\` from \`remote/make-patches.py\` and \`src/mast\` against`,
    `Chromium **${pinned}** with ungoogled-chromium **${pinnedTag}**. Don't edit by hand; change the code`,
    `and re-run. \`./mast check-rebase <version>\` checks all of this against another release.`, '');
  L.push('| | |', '|---|---|',
    `| Edits to upstream files | ${edits.length} (in ${files.size} files) |`,
    ...[...byPatch].map(([p, es]) => `| ${code1(p)} | ${es.length} edits |`),
    `| Chromium headers included by \`src/mast\` | ${includes.size} |`,
    `| Upstream methods overridden | ${overrides.size} |`,
    `| Upstream GN deps | ${gnDeps.size} |`,
    `| ungoogled patches dropped / added | ${list('ungoogled-drop.list').length} / ${list('ungoogled-extra.list').length} |`, '');

  L.push('## 1. Edits to upstream files', '',
    'Each one is an anchored edit in `remote/make-patches.py`: it must match exactly once, or the',
    'patch run fails. "Why" is the comment above the edit.', '');
  for (const [patch, es] of byPatch) {
    L.push(`### ${patch}`, '', '| Upstream file | Line | Section | Why |', '|---|---|---|---|');
    for (const e of es) L.push(`| ${code1(e.path)} | [${e.line}](../remote/make-patches.py#L${e.line}) | ${md(section(e.line))} | ${md(why(e.line)) || ''} |`);
    L.push('');
  }

  L.push('## 2. Chromium code that `src/mast` builds against', '',
    'These break at compile time, not in the patch run. `check-rebase` compares each header',
    'between releases and flags changes to the names below.', '');
  L.push(`**Overridden methods** (${overrides.size}): ${[...overrides].sort().map(code1).join(', ')}`, '');
  const byDir = new Map();
  for (const h of [...includes.keys()].sort()) {
    const d = h.split('/').slice(0, 2).join('/');
    if (!byDir.has(d)) byDir.set(d, []);
    byDir.get(d).push(h);
  }
  L.push('<details><summary>Headers included, by directory</summary>', '', '| Header | Included by |', '|---|---|');
  for (const hs of byDir.values()) for (const h of hs) L.push(`| ${code1(h)} | ${[...includes.get(h)].sort().join(', ')} |`);
  L.push('', '</details>', '');
  L.push('<details><summary>Upstream GN deps</summary>', '', '| Dep | Named in |', '|---|---|');
  for (const [d, fs_] of [...gnDeps].sort()) L.push(`| ${code1(d)} | ${[...fs_].sort().join(', ')} |`);
  L.push('', '</details>', '');

  L.push('## 3. ungoogled-chromium', '',
    'Mast builds on ungoogled-chromium\'s patches, with these changes (`patches/*.list`):', '');
  for (const n of list('ungoogled-drop.list')) L.push(`- not applied: ${code1(n)}`);
  for (const n of list('ungoogled-extra.list')) L.push(`- also applied (from extra/): ${code1(n)}`);
  for (const n of list('substitution-exclude.list')) L.push(`- no domain substitution in: ${code1(n)}`);
  const unp = makePatches.filter((l) => /\bunpatch\(/.test(l) && !/^def /.test(l)).length;
  if (unp) L.push(`- partly undone by \`unpatch()\` in make-patches.py (${unp} places)`);
  L.push('');
  fs.writeFileSync(path.join(root, 'docs/UPSTREAM.md'), L.join('\n'));
  console.log('docs/UPSTREAM.md updated');
}

// ------------------------------------------------------------------ report

function report(work, to, toTag) {
  const fromEdits = JSON.parse(read(path.join(work, 'from/edits.json')));
  const toEdits = JSON.parse(read(path.join(work, 'to/edits.json')));
  const failed = toEdits.filter((e) => e.error);
  const patched = [...new Set(fromEdits.map((e) => e.path))].sort();
  const headers = [...includes.keys()].sort();
  const gn = [...new Set([...gnDeps.keys()].map((d) => gnTarget(d).file))].sort();
  const want = [...new Set([...patched, ...headers, ...gn])];
  process.stderr.write(`reading ${want.length} files at ${pinned} and ${to}\n`);
  const A = baseFiles(pinned, pinnedTag, want), B = baseFiles(to, toTag, want);

  // 1. Edits that fail on the target.
  // 2. Upstream changes next to edits that still apply (the anchor matched, the code around moved).
  const ourH = ourHunks(path.join(work, 'from'));
  const closeCalls = [], depLists = [], elsewhere = [];
  for (const f of patched) {
    if (!A.get(f) || !B.get(f) || failed.some((e) => e.path === f)) continue;
    const theirs = hunks(A.get(f), B.get(f));
    if (!theirs.length) continue;
    const mine = ourH.get(f) ?? [];
    const hits = theirs.filter((t) => mine.some((m) => near(t, m)));
    // A BUILD.gn edit only adds a dep to a list; neighbours changing there can't conflict
    // (a target that disappears is the GN check's job).
    const item = { f, hits, all: theirs.length };
    (!hits.length ? elsewhere : f.endsWith('.gn') ? depLists : closeCalls).push(item);
  }

  // 3. Headers: gone, names Mast uses that vanish, overridden methods whose lines changed,
  // called methods whose lines changed.
  const gone = [], high = [], medium = [], low = [];
  for (const h of headers) {
    const a = A.get(h), b = B.get(h);
    if (!a) continue;  // generated or another repo: not checked
    if (!b) { gone.push(h); continue; }
    const ta = stripComments(read(a)), tb = stripComments(read(b));
    if (ta === tb) continue;
    const la = new Set(ta.split('\n').map((l) => l.trim()).filter(Boolean));
    const lb = new Set(tb.split('\n').map((l) => l.trim()).filter(Boolean));
    const removed = [...la].filter((l) => !lb.has(l)), added = [...lb].filter((l) => !la.has(l));
    const has = (t, n) => new RegExp(`\\b${n}\\b`).test(t);
    const lines = (n) => [...removed.filter((l) => has(l, n)).map((l) => `- ${l}`), ...added.filter((l) => has(l, n)).map((l) => `+ ${l}`)].slice(0, 6);
    for (const n of [...overrides, ...calls, ...constants]) {
      if (!has(ta, n) || !removed.some((l) => has(l, n))) continue;
      const users = [...includes.get(h)].filter((f) => has(code.get(path.join(srcDir, f)) ?? '', n));
      if (!users.length) continue;
      const item = { h, n, users, lines: lines(n) };
      if (!has(tb, n)) high.push(item);
      else if (overrides.has(n)) medium.push(item);
      else low.push(item);
    }
  }

  // 4. GN deps whose target disappears.
  const gnGone = [];
  for (const [d, users] of gnDeps) {
    const { file, name } = gnTarget(d);
    const re = new RegExp(`\\(\\s*"${name.replace(/[.*+?^${}()|[\]\\]/g, '\\$&')}"\\s*\\)`);
    const a = A.get(file), b = B.get(file);
    const inA = a && re.test(read(a)), inB = b && re.test(read(b));
    if (inA && !inB) gnGone.push({ d, users: [...users] });
  }

  // 5. ungoogled-chromium.
  const ugA = ungoogled(pinned, pinnedTag), ugB = ungoogled(to, toTag);
  const lists = ['ungoogled-drop.list', 'ungoogled-extra.list'].flatMap((l) =>
    read(path.join(root, 'patches', l)).split('\n').map((x) => x.replace(/#.*/, '').trim()).filter(Boolean)
      .map((n) => ({ l, n, ok: ugB.all.some((s) => path.basename(s) === path.basename(n)) })));
  const ugRemoved = ugA.applied.filter((n) => !ugB.applied.includes(n));
  const ugAdded = ugB.applied.filter((n) => !ugA.applied.includes(n));
  const overlap = (names, ug) => names.map((n) => ({ n, files: ug.touches(n).filter((f) => patched.includes(f)) }));
  const ugRm = overlap(ugRemoved, ugA), ugAdd = overlap(ugAdded, ugB);

  // The report.
  const findEdits = (f) => fromEdits.filter((e) => e.path === f).map((e) => `[make-patches.py:${e.line}](../../remote/make-patches.py#L${e.line})`);
  const must = failed.length + gone.length + high.length + gnGone.length + lists.filter((x) => !x.ok).length;
  const review = closeCalls.length + medium.length + [...ugRm, ...ugAdd].filter((x) => x.files.length).length;
  const L = [];
  L.push(`# Rebase check: ${pinned} → ${to}`, '');
  L.push(`Generated ${new Date().toISOString().slice(0, 10)} by \`./mast check-rebase ${to}${toTag === `${to}-1` ? '' : ' ' + toTag}\`,`,
    `against ungoogled-chromium **${toTag}** (now on **${pinnedTag}**). Local and free; see`,
    '[UPSTREAM.md](../UPSTREAM.md) for what is checked.', '');
  L.push('| | Count | |', '|---|---|---|',
    `| Patch edits that fail | ${failed.length} of ${toEdits.length} | must fix |`,
    `| Headers we include that are gone | ${gone.length} | must fix |`,
    `| Upstream names we use that are gone | ${high.length} | must fix |`,
    `| GN deps that are gone | ${gnGone.length} | must fix |`,
    `| ungoogled patches we list that are gone | ${lists.filter((x) => !x.ok).length} | must fix |`,
    `| Upstream changes right next to our edits | ${closeCalls.length} files | review |`,
    `| Overridden methods whose declaration changed | ${medium.length} | review |`,
    `| ungoogled patches added/removed in files we patch | ${[...ugRm, ...ugAdd].filter((x) => x.files.length).length} | review |`,
    `| Called methods whose declaration changed | ${low.length} | glance |`,
    `| BUILD.gn dep lists changed next to ours | ${depLists.length} | fyi |`,
    `| Patched files changed away from our edits | ${elsewhere.length} | fyi |`, '');
  L.push(must ? `**${must} to fix before building, ${review} to review.**` : `**Nothing to fix before building; ${review} to review.**`, '');

  L.push('## Must fix', '');
  if (failed.length) {
    L.push('### Patch edits that fail', '', '| Edit | Patch | Upstream file | Error |', '|---|---|---|---|');
    for (const e of failed) L.push(`| [make-patches.py:${e.line}](../../remote/make-patches.py#L${e.line}) | ${e.patch} | ${code1(e.path)} | ${md(e.error)} |`);
    L.push('');
  }
  if (gone.length) L.push('### Headers gone', '', ...gone.map((h) => `- ${code1(h)}, included by ${[...includes.get(h)].join(', ')}`), '');
  if (high.length) {
    L.push('### Names gone from a header we include', '', 'Renamed, removed, or moved to another header.', '');
    for (const x of high) L.push(`- ${code1(x.n)} from ${code1(x.h)}, used in ${x.users.join(', ')}`, '', '  ```diff', ...x.lines.map((l) => '  ' + l), '  ```', '');
  }
  if (gnGone.length) L.push('### GN deps gone', '', ...gnGone.map((x) => `- ${code1(x.d)}, in ${x.users.join(', ')}`), '');
  for (const x of lists.filter((x) => !x.ok)) L.push(`- ${code1(x.n)} (in patches/${x.l}) isn't in ungoogled's ${toTag} series`);
  if (!must) L.push('Nothing.', '');

  L.push('## Review', '');
  if (closeCalls.length) {
    L.push('### Upstream changed right next to our edits', '',
      'The anchors still match, but the code within a few lines of our change moved. Read the',
      'upstream diff before trusting the edit.', '');
    for (const x of closeCalls) {
      L.push(`#### ${code1(x.f)}`, '', `Our edit: ${findEdits(x.f).join(', ')}. Upstream, ${x.hits.length} of ${x.all} changed regions are next to it:`, '', '```diff');
      const lines = x.hits.flatMap((h) => [`@@ line ${h[0]}`, ...h.text]);
      L.push(...lines.slice(0, 40), ...(lines.length > 40 ? [`… ${lines.length - 40} more lines`] : []), '```', '');
    }
  }
  if (medium.length) {
    L.push('### Overridden methods whose declaration changed', '');
    for (const x of medium) L.push(`- ${code1(x.n)} in ${code1(x.h)}, overridden in ${x.users.join(', ')}`, '', '  ```diff', ...x.lines.map((l) => '  ' + l), '  ```', '');
  }
  const ugAny = [...ugRm.map((x) => ({ ...x, how: 'removed' })), ...ugAdd.map((x) => ({ ...x, how: 'added' }))];
  if (ugAny.length) {
    L.push('### ungoogled-chromium series changes', '');
    for (const x of ugAny) L.push(`- ${x.how}: ${code1(x.n)}${x.files.length ? `; touches files we patch: ${x.files.map(code1).join(', ')}` : ''}`);
    L.push('', 'A removed patch: either upstream dropped what it changed, or it comes back in Mast. Check', 'which before the build (first run, settings, network requests).', '');
  }
  if (!review && !ugAny.length) L.push('Nothing.', '');

  if (low.length) {
    L.push('## Glance', '', '<details><summary>Methods we call whose declaration lines changed (' + low.length + ')</summary>', '',
      'Usually an added overload or a parameter we don\'t pass; a compile error if not.', '');
    for (const x of low) L.push(`- ${code1(x.n)} in ${code1(x.h)} (${x.users.join(', ')})`, '', '  ```diff', ...x.lines.map((l) => '  ' + l), '  ```', '');
    L.push('</details>', '');
  }
  if (depLists.length) {
    L.push('<details><summary>BUILD.gn dep lists that changed next to the dep we add (' + depLists.length + ')</summary>', '');
    for (const x of depLists) L.push(`- ${code1(x.f)}`);
    L.push('', '</details>', '');
  }
  if (elsewhere.length) {
    L.push('<details><summary>Patched files that changed away from our edits (' + elsewhere.length + ')</summary>', '');
    for (const x of elsewhere) L.push(`- ${code1(x.f)} (${x.all} regions)`);
    L.push('', '</details>', '');
  }
  L.push('## Not checked here', '',
    '- Whether it compiles: the build is the proof. Build the hooked files first (see ROADMAP §2).',
    '- Behaviour inside Chromium (a hook that still applies but no longer runs, or runs twice).',
    '- UI order the screenshots rely on (`remote/shots.py`: the tab menu is reached with Down keys).',
    '- Headers outside Chromium\'s main source (generated mojom, Skia), which aren\'t fetched.', '');
  const out = path.join(root, 'docs/rebase', `${to}.md`);
  fs.mkdirSync(path.dirname(out), { recursive: true });
  fs.writeFileSync(out, L.join('\n'));
  console.log(`${path.relative(root, out)}: ${must} to fix, ${review} to review, ${low.length} to glance at`);
}

const [mode, ...rest] = process.argv.slice(2);
if (mode === 'inventory') inventory(rest[0]);
else if (mode === 'report') report(rest[0], rest[1], rest[2] ?? `${rest[1]}-1`);
else { console.error('usage: rebase-check.mjs inventory <work> | report <work> <version> [ungoogled tag]'); process.exit(2); }
