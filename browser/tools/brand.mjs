#!/usr/bin/env node
// Mast brand assets, from one source. Every logo file in the repo is written by this script;
// change the mark here and run it again, never edit a PNG by hand.
//
//   node browser/tools/brand.mjs        (needs rsvg-convert on PATH)
//
// The system
//   Mark      Two sails on a 240 grid, unit g = 30. The gap between them is the mast: 1g wide.
//             Mainsail 3g x 6.5g, jib the same shape at 10/13. Each outer edge is one circular
//             arc that leaves the foot vertically. Sharp corners, one flat colour.
//   Colour    Black and white only. Ember (#ff8f63) is the UI's signal colour and never
//             appears in the logo.
//   Icon      White sails on a flat black tile (corner radius 11%). No gradient, no outline.
//   Glyph     The bare sails in the current text colour: toolbar, chips, Mast's own pages.
//   Outline   The same sails as an inside stroke (design/brand only; tabs use the glyph).
//   Wordmark  "Mast", drawn from Host Grotesk SemiBold (OFL) and kept as outlines; the t is
//             raised to cap height and cut at the angle of the mainsail's head.
//   Lockup    Sails + wordmark on one baseline. Cap height = the jib's height, so the mainsail
//             stands 1.3 caps tall. The space between them is 2g.

import { execFileSync } from 'node:child_process';
import { mkdirSync, mkdtempSync, readFileSync, rmSync, writeFileSync } from 'node:fs';
import { tmpdir } from 'node:os';
import { dirname, join } from 'node:path';
import { fileURLToPath } from 'node:url';
import { inflateSync } from 'node:zlib';

const ROOT = join(dirname(fileURLToPath(import.meta.url)), '..', '..');
const OVERRIDES = join(ROOT, 'browser/src/overrides');
const BRAND = join(ROOT, 'design/brand');
const TMP = mkdtempSync(join(tmpdir(), 'mast-brand-'));

// ---------------------------------------------------------------------------------------
// The mark

const G = 30; // the mast
const BASE = 210;
const MAIN = { x: 120, w: 3 * G, h: 6.5 * G, dir: 1 };
const K = 10 / 13;
const JIB = { x: MAIN.x - G, w: MAIN.w * K, h: MAIN.h * K, dir: -1 };
const INK = '#000000';
const PAPER = '#ffffff';

const n = (v) => +v.toFixed(3);
const radius = (s) => (s.h * s.h + s.w * s.w) / (2 * s.w);

// One sail as an SVG path: mast edge, foot, then the arc back up to the head. `x` is where
// its mast edge stands and `base` where its foot sits, both already scaled.
function sailPath(s, k = 1, x = s.x * k, base = BASE * k) {
	const r = n(radius(s) * k);
	const head = `${n(x)} ${n(base - s.h * k)}`;
	return `M${head}V${n(base)}H${n(x + s.dir * s.w * k)}A${r} ${r} 0 0 ${s.dir > 0 ? 0 : 1} ${head}Z`;
}
const MARK = sailPath(JIB) + sailPath(MAIN);

// The mark at a pixel size: scaled by k and moved by (ox, oy), then the mast's two edges and
// the foot are put on whole pixels so small sizes stay crisp. The mast is never under 1px.
function pixelMark(k, ox = 0, oy = 0) {
	const main = Math.round(ox + MAIN.x * k);
	const jib = main - Math.max(1, Math.round(G * k - 0.01));
	const base = Math.round(oy + BASE * k);
	return sailPath(JIB, k, jib, base) + sailPath(MAIN, k, main, base);
}

// The same sail for Chromium's .icon format, the arc as one cubic (it spans under 50 degrees).
function sailIcon(s, k) {
	const r = radius(s);
	const foot = [s.x + s.dir * s.w, BASE];
	const head = [s.x, BASE - s.h];
	const c = [foot[0] - s.dir * r, BASE];
	const theta = Math.asin(s.h / r);
	const len = (4 / 3) * Math.tan(theta / 4) * r;
	const c1 = [foot[0], foot[1] - len]; // the arc leaves the foot straight up
	const t = [(head[1] - c[1]) / r, -(head[0] - c[0]) / r]; // unit tangent at the head
	const sign = t[1] > 0 ? 1 : -1; // pointing back down towards the foot
	const c2 = [head[0] + sign * t[0] * len, head[1] + sign * t[1] * len];
	const f = (v) => +(v * k).toFixed(2);
	return [
		`MOVE_TO, ${f(head[0])}, ${f(head[1])},`,
		`V_LINE_TO, ${f(BASE)},`,
		`H_LINE_TO, ${f(foot[0])},`,
		`CUBIC_TO, ${f(c1[0])}, ${f(c1[1])}, ${f(c2[0])}, ${f(c2[1])}, ${f(head[0])}, ${f(head[1])},`,
		'CLOSE'
	].join('\n');
}

// ---------------------------------------------------------------------------------------
// The wordmark, in the mark's units: baseline 210, cap height 150 (the jib's height), ink
// starting 2g after the mainsail. Outlines only, so nothing here depends on a font file.

const WORD =
	'M295.71 210L270 210L270 60L300.64 60L350.14 175.71L398.79 60L429.21 60L429.21 210L403.5 210L403.5 107.36L359.79 210L339.64 210L295.71 107.36L295.71 210Z' +
	'M484.93 212.57L484.93 212.57Q472.71 212.57 464.14 208.18Q455.57 203.79 451.07 196.5Q446.57 189.21 446.57 180.64L446.57 180.64Q446.57 163.93 458.68 154.71Q470.79 145.5 490.5 145.5L490.5 145.5Q499.5 145.5 506.46 146.89Q513.43 148.29 517.71 149.57L517.71 149.57L517.71 144.64Q517.71 132.86 511.18 127.29Q504.64 121.71 494.14 121.71L494.14 121.71Q486.43 121.71 480.43 124.82Q474.43 127.93 472.71 136.07L472.71 136.07L448.29 136.07Q449.57 124.5 456 116.79Q462.43 109.07 472.39 105.11Q482.36 101.14 494.14 101.14L494.14 101.14Q517.5 101.14 530.46 112.61Q543.43 124.07 543.43 144.64L543.43 144.64L543.43 170.57Q543.43 178.71 544.39 182.46Q545.36 186.21 547.61 187.18Q549.86 188.14 553.5 188.14L553.5 188.14L554.57 188.14L554.57 210L548.79 210Q537.86 210 530.46 206.89Q523.07 203.79 520.07 194.57L520.07 194.57Q514.71 202.29 505.93 207.43Q497.14 212.57 484.93 212.57ZM490.5 192.64L490.5 192.64Q498 192.64 503.36 189.11Q508.71 185.57 512.04 179.68Q515.36 173.79 516.43 166.71L516.43 166.71Q511.93 165.43 505.71 164.14Q499.5 162.86 493.07 162.86L493.07 162.86Q483.43 162.86 477.96 167.14Q472.5 171.43 472.5 178.71L472.5 178.71Q472.5 184.71 477.32 188.68Q482.14 192.64 490.5 192.64Z' +
	'M611.14 212.57L611.14 212.57Q590.57 212.57 577.07 202.93Q563.57 193.29 561.64 176.36L561.64 176.36L588 176.36Q589.5 183.86 596.04 188.25Q602.57 192.64 612 192.64L612 192.64Q621.64 192.64 626.89 188.89Q632.14 185.14 632.14 179.57L632.14 179.57Q632.14 175.07 628.61 172.61Q625.07 170.14 616.07 168.86L616.07 168.86Q612.64 168.21 607.61 167.46Q602.57 166.71 598.93 165.86L598.93 165.86Q590.57 164.36 583.29 160.61Q576 156.86 571.61 150.43Q567.21 144 567.21 134.57L567.21 134.57Q567.21 120.43 577.82 110.79Q588.43 101.14 608.14 101.14L608.14 101.14Q627.86 101.14 639.64 109.82Q651.43 118.5 653.14 135.21L653.14 135.21L628.07 135.21Q627.43 128.14 621.75 124.71Q616.07 121.29 608.14 121.29L608.14 121.29Q599.57 121.29 594.86 124.61Q590.14 127.93 590.14 132.86L590.14 132.86Q590.14 141.64 604.93 144.21L604.93 144.21Q607.07 144.64 610.39 145.18Q613.71 145.71 617.14 146.25Q620.57 146.79 622.93 147.21L622.93 147.21Q628.71 147.86 634.61 149.79Q640.5 151.71 645.43 155.04Q650.36 158.36 653.25 163.71Q656.14 169.07 656.14 177.21L656.14 177.21Q656.14 187.71 650.89 195.64Q645.64 203.57 635.57 208.07Q625.5 212.57 611.14 212.57Z' +
	'M731.36 210L710.57 210Q700.29 210 693.21 207.54Q686.14 205.07 682.5 198.21Q678.86 191.36 678.86 178.29L678.86 178.29L678.86 125.36L660.64 125.36L660.64 103.71L678.86 103.71L678.86 60L704.57 81.86L704.57 103.71L732.43 103.71L732.43 125.36L704.57 125.36L704.57 174.86Q704.57 180.64 705.75 183.43Q706.93 186.21 709.93 187.18Q712.93 188.14 718.71 188.14L718.71 188.14L731.36 188.14L731.36 210Z';
const LOCKUP = { x: 20, y: 14, w: 713, h: 200 }; // the box that holds sails and word

// ---------------------------------------------------------------------------------------
// SVG builders

const svg = (w, h, viewBox, body) =>
	`<svg xmlns="http://www.w3.org/2000/svg" width="${w}" height="${h}" viewBox="${viewBox}">${body}</svg>\n`;

const markSvg = (size, fill) =>
	svg(size, size, `0 0 ${size} ${size}`, `<path d="${pixelMark(size / 240)}" fill="${fill}"/>`);

// The outline is drawn inside the shape, so its outer edge is exactly the mark's.
const outlineSvg = (size, stroke, weight = 18) =>
	svg(
		size,
		size,
		'0 0 240 240',
		`<clipPath id="m"><path d="${MARK}"/></clipPath>` +
			`<path d="${MARK}" fill="none" stroke="${stroke}" stroke-width="${weight * 2}" clip-path="url(#m)"/>`
	);

// The tile. `region` is how many pixels the 240 grid covers; it is picked per size so the
// mast's edges land on whole pixels (the grid is 8 masts wide).
const REGION = { 16: 12, 22: 16, 24: 16, 32: 24, 48: 32, 64: 48, 128: 96, 256: 192 };
function iconSvg(size, bg = INK, fg = PAPER) {
	const region = REGION[size] ?? size * 0.75;
	const k = region / 240;
	const off = (size - region) / 2;
	return svg(
		size,
		size,
		`0 0 ${size} ${size}`,
		`<rect width="${size}" height="${size}" rx="${n(size * 0.11)}" fill="${bg}"/>` +
			`<path d="${pixelMark(k, off, off)}" fill="${fg}"/>`
	);
}

// Sails + word, fitted into a w x h image: left-aligned, centred vertically.
function lockupSvg(w, h, fill, pad = 1) {
	const k = Math.min((h - 2 * pad) / LOCKUP.h, w / LOCKUP.w);
	const ty = (h - LOCKUP.h * k) / 2 - LOCKUP.y * k;
	return svg(
		w,
		h,
		`0 0 ${w} ${h}`,
		`<g transform="translate(${n(-LOCKUP.x * k)} ${n(ty)}) scale(${n(k)})" fill="${fill}"><path d="${MARK}"/><path d="${WORD}"/></g>`
	);
}

// ---------------------------------------------------------------------------------------
// Output

function write(path, text) {
	mkdirSync(dirname(path), { recursive: true });
	writeFileSync(path, text);
}

let count = 0;
function png(path, source) {
	const src = join(TMP, `${count++}.svg`);
	writeFileSync(src, source);
	mkdirSync(dirname(path), { recursive: true });
	execFileSync('rsvg-convert', [src, '-o', path]);
}

// Just enough PNG to read back what rsvg-convert wrote (8-bit RGBA, not interlaced).
function pixels(path) {
	const b = readFileSync(path);
	const w = b.readUInt32BE(16);
	const h = b.readUInt32BE(20);
	const idat = [];
	for (let p = 8; p < b.length; ) {
		const len = b.readUInt32BE(p);
		if (b.toString('latin1', p + 4, p + 8) === 'IDAT') idat.push(b.subarray(p + 8, p + 8 + len));
		p += 12 + len;
	}
	const raw = inflateSync(Buffer.concat(idat));
	const out = Buffer.alloc(w * h * 4);
	const stride = w * 4;
	for (let y = 0; y < h; y++) {
		const filter = raw[y * (stride + 1)];
		for (let x = 0; x < stride; x++) {
			const v = raw[y * (stride + 1) + 1 + x];
			const a = x >= 4 ? out[y * stride + x - 4] : 0;
			const up = y ? out[(y - 1) * stride + x] : 0;
			const c = x >= 4 && y ? out[(y - 1) * stride + x - 4] : 0;
			const pa = Math.abs(up - c);
			const pb = Math.abs(a - c);
			const pc = Math.abs(a + up - 2 * c);
			const paeth = pa <= pb && pa <= pc ? a : pb <= pc ? up : c;
			out[y * stride + x] = (v + [0, a, up, (a + up) >> 1, paeth][filter]) & 255;
		}
	}
	return { w, h, data: out };
}

// XPM has no partial transparency: a pixel is either the tile (in 16 greys) or nothing.
function xpm(path, name, source) {
	const tmp = join(TMP, 'xpm.png');
	png(tmp, source);
	const { w, h, data } = pixels(tmp);
	const chars = ' .+@#$%&*=-;>,)!';
	const rows = [];
	for (let y = 0; y < h; y++) {
		let row = '';
		for (let x = 0; x < w; x++) {
			const i = (y * w + x) * 4;
			row += data[i + 3] < 128 ? ' ' : chars[1 + Math.round((data[i] / 255) * 14)];
		}
		rows.push(`"${row}"`);
	}
	const colours = ['"  c None"'];
	for (let i = 0; i < 15; i++) {
		const hex = Math.round((i / 14) * 255).toString(16).padStart(2, '0').toUpperCase();
		colours.push(`"${chars[1 + i]} c #${hex}${hex}${hex}"`);
	}
	write(
		path,
		`/* XPM */\nstatic char *${name}[] = {\n/* columns rows colors chars-per-pixel */\n"${w} ${h} 16 1 ",\n` +
			`${colours.join(',\n')},\n/* pixels */\n${rows.join(',\n')}\n};\n`
	);
}

// The source drawings, for anyone who needs the logo outside the browser.
write(join(BRAND, 'mark.svg'), markSvg(240, INK));
write(join(BRAND, 'mark-white.svg'), markSvg(240, PAPER));
write(join(BRAND, 'mark-outline.svg'), outlineSvg(240, INK));
write(join(BRAND, 'icon.svg'), iconSvg(256));
write(join(BRAND, 'icon-light.svg'), iconSvg(256, PAPER, INK));
write(join(BRAND, 'lockup.svg'), lockupSvg(LOCKUP.w, LOCKUP.h, INK, 0));
write(join(BRAND, 'lockup-white.svg'), lockupSvg(LOCKUP.w, LOCKUP.h, PAPER, 0));

// The app icon, at every size Chromium and the installer ask for.
const theme = join(OVERRIDES, 'chrome/app/theme');
for (const s of [16, 24, 48, 64, 128, 256]) png(join(theme, `chromium/product_logo_${s}.png`), iconSvg(s));
for (const s of [24, 48, 64, 128, 256]) png(join(theme, `chromium/linux/product_logo_${s}.png`), iconSvg(s));
xpm(join(theme, 'chromium/linux/product_logo_32.xpm'), 'product_logo_32', iconSvg(32));
for (const [dir, scale] of [['default_100_percent', 1], ['default_200_percent', 2]]) {
	for (const s of [16, 32]) {
		png(join(theme, dir, `chromium/product_logo_${s}.png`), iconSvg(s * scale));
		if (scale === 1) png(join(theme, dir, `chromium/linux/product_logo_${s}.png`), iconSvg(s));
	}
	png(join(OVERRIDES, 'components/resources', dir, 'chromium/favicon_product.png'), iconSvg(16 * scale));

	// Sails + word: dark for light surfaces, white for dark ones.
	png(join(theme, dir, 'chromium/product_logo_name_22.png'), lockupSvg(97 * scale, 22 * scale, INK, scale));
	png(join(theme, dir, 'chromium/product_logo_name_white_22.png'), lockupSvg(97 * scale, 22 * scale, PAPER, scale));
	const res = join(OVERRIDES, 'components/resources', dir, 'chromium');
	png(join(res, 'product_logo.png'), lockupSvg(171 * scale, 32 * scale, INK, 2 * scale));
	png(join(res, 'product_logo_white.png'), lockupSvg(171 * scale, 32 * scale, PAPER, 2 * scale));

	// A new tab and Mast's own pages: the filled sails in the tabs' text colour (Onyx's
	// #ededed); the one-row window recolours them to each tab's own title colour.
	png(join(theme, dir, 'common/favicon_ntp.png'), markSvg(16 * scale, '#ededed'));
}
png(join(theme, 'chromium/product_logo_22_mono.png'), markSvg(22, INK));

// Mast's own pages (blocked, chrome://mast, welcome) use the same icon as a new tab,
// inlined as kIcon in mast_pages.cc, so a tab never switches between two icons.
{
	const pages = join(ROOT, 'browser/src/mast/browser/mast_pages.cc');
	const ntp = readFileSync(join(theme, 'default_200_percent/common/favicon_ntp.png')).toString('base64');
	const text = readFileSync(pages, 'utf8');
	const re = /(constexpr char kIcon\[\] =\s*R"\(<link rel="icon" href="data:image\/png;base64,)[A-Za-z0-9+/=]+/;
	if (!re.test(text)) throw new Error('kIcon not found in mast_pages.cc');
	write(pages, text.replace(re, `$1${ntp}`));
}

// The glyph inside the browser's own UI.
const icon =
	'// Copyright 2026 The Mast Authors\n' +
	'// Mast sails (monochrome; takes the current icon colour).\n' +
	'// Written by browser/tools/brand.mjs.\n\n' +
	`CANVAS_DIMENSIONS, 24,\n${sailIcon(JIB, 0.1)},\n${sailIcon(MAIN, 0.1)}\n`;
for (const f of [
	'chrome/app/vector_icons/browser_logo_old.icon',
	'chrome/app/vector_icons/chrome_product.icon',
	'components/omnibox/browser/vector_icons/chrome_product.icon',
	'components/omnibox/browser/vector_icons/product_chrome_refresh_old.icon',
	'components/omnibox/browser/vector_icons/product_old.icon',
	'components/vector_icons/chromium/product.icon',
	'components/vector_icons/chromium/product_refresh.icon'
]) {
	write(join(OVERRIDES, f), icon);
}
write(
	join(OVERRIDES, 'ui/webui/resources/images/chrome_logo_dark.svg'),
	svg(24, 24, '0 0 24 24', `<path d="${sailPath(JIB, 0.1) + sailPath(MAIN, 0.1)}" fill="#fff"/>`)
);

// The demo's tab icon.
write(join(ROOT, 'demo/src/lib/assets/favicon.svg'), iconSvg(32));

rmSync(TMP, { recursive: true, force: true });
console.log(`mark   ${MARK}`);
console.log('brand assets written');
