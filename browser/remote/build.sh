#!/usr/bin/env bash
# Build Mast on the builder: sync Chromium to the pinned version (only when it changes),
# apply patches/series (only when they change), copy src/mast in, build, package.
set -euo pipefail
export PATH=$HOME/depot_tools:$PATH
M=$HOME/mast
C=$HOME/chromium
S=$C/src
OUT=$S/out/Mast
VERSION=$(cat "$M/chromium_version.txt")
say() { printf '\n== %s\n' "$*"; }

cd "$C"

# 1. Chromium version
if [ "$(cat .mast-version 2>/dev/null)" != "$VERSION" ]; then
	say "Syncing Chromium $VERSION (slow the first time)"
	git -C src reset --hard -q || true
	rm -f .mast-patches
	# Fetch only this tag, then sync by commit hash: gclient skips its own fetch when the
	# commit is already present (a tag ref makes it fetch every branch with full history).
	git -C src fetch --depth 1 origin "+refs/tags/$VERSION:refs/tags/$VERSION"
	REV=$(git -C src rev-parse "refs/tags/$VERSION^{commit}")
	gclient sync --nohooks --no-history -D --force --revision "src@$REV"
	gclient runhooks
	echo "$VERSION" > .mast-version
fi

# 2. Patches — re-applied from a clean tree only when the series or a patch changes
mapfile -t PATCHES < <(sed -e 's/#.*//' -e '/^[[:space:]]*$/d' "$M/patches/series")
SUM=$( { cat "$M/patches/series"; for p in "${PATCHES[@]}"; do cat "$M/patches/$p"; done; } | sha256sum | cut -d' ' -f1)
if [ "$(cat .mast-patches 2>/dev/null)" != "$SUM" ]; then
	say "Applying ${#PATCHES[@]} patch(es)"
	git -C src reset --hard -q
	for p in "${PATCHES[@]}"; do
		echo "  $p"
		git -C src apply -p1 "$M/patches/$p"
	done
	echo "$SUM" > .mast-patches
fi

# 3. Mast's own code (rsync keeps mtimes, so unchanged files don't rebuild), and
# whole-file overrides of upstream files (branding, icons). --checksum so an
# override only touches mtimes when its content really differs.
mkdir -p "$S/mast"
rsync -a --delete "$M/src/mast/" "$S/mast/"
if [ -d "$M/src/overrides" ]; then
	rsync -a --checksum "$M/src/overrides/" "$S/"
fi
# Mast's HTML pages -> C++ strings, in the generated-sources dir so the rsync of
# src/mast above never deletes them (unchanged pages keep their mtime).
python3 "$M/remote/embed-pages.py" "$S/mast/browser/resources" "$OUT/gen/mast/browser/resources"
# Product name in every UI string file (only rewrites files that change).
python3 "$M/remote/substitute-names.py" "$S"

# 4. Configure + build
mkdir -p "$OUT"
cmp -s "$M/remote/args.gn" "$OUT/args.gn" || cp "$M/remote/args.gn" "$OUT/args.gn"
cd "$S"
say "gn gen"
gn gen "$OUT"
say "Building chrome"
# -k 0: keep going past failures so one pass reports every error.
time autoninja -C "$OUT" -k 0 chrome

# 5. Package only what the browser needs at runtime
say "Packaging"
cd "$OUT"
keep=()
for f in chrome chrome_crashpad_handler icudtl.dat vk_swiftshader_icd.json locales MEIPreload \
	*.pak *.bin *.so *.so.*; do
	[ -e "$f" ] && keep+=("$f")
done
mkdir -p "$M/dist"
echo "$VERSION" > "$M/dist/VERSION"
tar --zstd -cf "$M/dist/mast-linux.tar.zst" "${keep[@]}"
ls -lh "$M/dist/mast-linux.tar.zst"
say "Done"
