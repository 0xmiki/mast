#!/usr/bin/env bash
# Turn the downloaded build (dist/mast) into tester downloads in dist/release/:
#   mast_amd64.deb            Debian, Ubuntu, Linux Mint, Pop!_OS, ...
#   mast.x86_64.rpm           Fedora, openSUSE, ...
#   mast-linux-x86_64.tar.xz  everything else (install.sh unpacks it into ~/.local)
#   install.sh                the one-line installer, which picks one of the above
#   SHA256SUMS                checksums install.sh verifies, plus the version
# File names stay the same across versions, so a "latest" download link always works.
# Runs locally in seconds to minutes; no builder.
set -euo pipefail
export LC_ALL=C # date, sort: Gregorian dates and plain byte order whatever the desktop locale
ROOT=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
# shellcheck source=../mast.env
source "$ROOT/mast.env"
P=$ROOT/packaging
SRC=$ROOT/dist/mast
OUT=$ROOT/dist/release
STAGE=$ROOT/dist/.stage
ICONS=$ROOT/src/overrides/chrome/app/theme/chromium

die() { echo "mast package: $*" >&2; exit 1; }
log() { printf '\033[1;33m›\033[0m %s\n' "$*"; }

[[ -x $SRC/chrome ]] || die "no build in dist/mast. Run: ./mast build"
[[ -n ${MAST_DOWNLOAD_URL:-} ]] || die "set MAST_DOWNLOAD_URL in mast.env (where testers download from)"
command -v readelf >/dev/null || die "readelf not found (binutils)"
if ! command -v nfpm >/dev/null; then
	exec nix --extra-experimental-features 'nix-command flakes' shell nixpkgs#nfpm -c "$0" "$@"
fi

# Chromium version plus the build's time, so every test build upgrades the last one.
VERSION=$(cat "$ROOT/dist/VERSION")
RELEASE=$(date -u -r "$ROOT/dist/mast-linux.tar.zst" +%Y%m%d%H%M)
log "packaging Mast $VERSION-$RELEASE"

# Stage: the browser, plus its launcher file and icons beside it (the tarball install
# reads them from there; Chrome ships its icons beside the browser the same way).
rm -rf "$STAGE" "$OUT"
mkdir -p "$STAGE/mast" "$STAGE/share" "$OUT"
cp -a "$SRC/." "$STAGE/mast/"
cp "$P/mast.desktop" "$STAGE/mast/"
for s in 24 48 64 128 256; do
	cp "$ICONS/product_logo_$s.png" "$STAGE/mast/"
done
SANDBOX=""
if [[ -e $STAGE/mast/chrome-sandbox ]]; then
	# Packaged separately so it can be given the setuid bit.
	mv "$STAGE/mast/chrome-sandbox" "$STAGE/share/chrome-sandbox"
	SANDBOX=1
else
	log "warning: this build has no chrome-sandbox (built before build.sh shipped it)."
	log "         Ubuntu 24.04+ / Mint 22 will still start Mast through the AppArmor profile."
fi

# rpm: depend on the exact libraries the browser links (distro-neutral, works on Fedora
# and openSUSE alike). GTK is loaded at runtime, so it's added by hand.
mapfile -t RPM_DEPS < <(readelf -d "$SRC/chrome" | sed -n 's/.*Shared library: \[\(.*\)\]/\1/p' |
	grep -vE '^(libc|libm|libdl|libpthread|librt|libgcc_s|ld-linux-x86-64)\.so' |
	sed 's/$/()(64bit)/'; echo 'libgtk-3.so.0()(64bit)'; echo xdg-utils)

# deb: the packages behind those libraries. Ubuntu 24.04's renamed "t64" packages still
# provide these names.
DEB_DEPS=(libasound2 libatk-bridge2.0-0 libatk1.0-0 libatspi2.0-0 libcairo2 libcups2 libdbus-1-3
	libexpat1 libgbm1 libglib2.0-0 libgtk-3-0 libnspr4 libnss3 libpango-1.0-0 libudev1 libx11-6
	libxcb1 libxcomposite1 libxdamage1 libxext6 libxfixes3 libxkbcommon0 libxrandr2 xdg-utils)

yaml_list() { for d in "$@"; do printf '      - "%s"\n' "$d"; done; }
{
	cat <<EOF
name: mast
arch: amd64
platform: linux
version: "$VERSION"
release: "$RELEASE"
version_schema: none
maintainer: "Mast"
description: |
  A deep-work browser: one button blocks every known distraction for a set time,
  and nothing inside the browser can stop it early. Built on Chromium.
section: web
priority: optional
contents:
  - src: $STAGE/mast
    dst: /opt/mast
    type: tree
  - src: /opt/mast/chrome
    dst: /usr/bin/mast
    type: symlink
  - src: $P/mast.desktop
    dst: /usr/share/applications/mast.desktop
EOF
	for s in 24 48 64 128 256; do
		printf '  - src: %s\n    dst: %s\n' "$ICONS/product_logo_$s.png" "/usr/share/icons/hicolor/${s}x$s/apps/mast.png"
	done
	if [[ -n $SANDBOX ]]; then
		printf '  - src: %s\n    dst: /opt/mast/chrome-sandbox\n    file_info:\n      mode: 04755\n' "$STAGE/share/chrome-sandbox"
	fi
	cat <<EOF
scripts:
  postinstall: $P/postinst.sh
  postremove: $P/postrm.sh
deb:
  compression: xz
rpm:
  compression: xz
overrides:
  deb:
    depends:
$(yaml_list "${DEB_DEPS[@]}")
  rpm:
    depends:
$(yaml_list "${RPM_DEPS[@]}")
EOF
} > "$STAGE/nfpm.yaml"

log "building the .deb"
nfpm package -f "$STAGE/nfpm.yaml" -p deb -t "$OUT/mast_amd64.deb"
log "building the .rpm"
nfpm package -f "$STAGE/nfpm.yaml" -p rpm -t "$OUT/mast.x86_64.rpm"

log "building the tarball"
[[ -n $SANDBOX ]] && cp -p "$STAGE/share/chrome-sandbox" "$STAGE/mast/chrome-sandbox"
XZ_OPT=-T0 tar -C "$STAGE/mast" --owner=0 --group=0 -cJf "$OUT/mast-linux-x86_64.tar.xz" .

sed "s|@BASE@|${MAST_DOWNLOAD_URL%/}|g" "$P/install.sh" > "$OUT/install.sh"
chmod 755 "$OUT/install.sh"
(cd "$OUT" && { echo "# version $VERSION-$RELEASE"; sha256sum mast_amd64.deb mast.x86_64.rpm mast-linux-x86_64.tar.xz install.sh; } > SHA256SUMS)
rm -rf "$STAGE"

log "done: dist/release/"
ls -lh "$OUT" | tail -n +2
log "testers run:  curl -fsSL ${MAST_DOWNLOAD_URL%/}/install.sh | sh"
