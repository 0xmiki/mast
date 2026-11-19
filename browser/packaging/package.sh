#!/usr/bin/env bash
# Turn the downloaded build (dist/mast) into a release in dist/release/:
#   mast_amd64.deb            Debian, Ubuntu, Linux Mint, Pop!_OS, ...
#   mast.x86_64.rpm           Fedora, openSUSE, ... (signed)
#   mast-linux-x86_64.tar.xz  everything else (install.sh unpacks it into ~/.local)
#   install.sh                the one-line installer, which picks one of the above
#   SHA256SUMS(.asc)          checksums install.sh verifies, plus the version; signed
#   Packages(.gz), Release, InRelease, Release.gpg   the apt source (signed)
#   rpm/repodata/             the dnf source, for GitHub Pages (signed)
#   latest.json               what tarball installs check for updates
# File names stay the same across versions, so a "latest" download link always works.
# Installing the .deb or .rpm also adds Mast's package source, so updates come with the
# system's own (packaging/mast.sources, mast.repo); publish.sh puts a release online.
#
# Signed with MAST_SIGNING_KEY (mast.env), which stays in your GnuPG: you're asked for its
# passphrase. MAST_UNSIGNED=1 skips signing (a local test build; don't publish it).
# Runs locally in seconds to minutes; no builder. Needs uv and nfpm (fetched once into
# ~/.cache/mast/tools when missing).
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
TOOLS=$HOME/.cache/mast/tools
NFPM_VERSION=2.49.0

die() { echo "mast package: $*" >&2; exit 1; }
log() { printf '\033[1;33m›\033[0m %s\n' "$*"; }

[[ -x $SRC/chrome ]] || die "no build in dist/mast. Run: ./mast build"
[[ -n ${MAST_DOWNLOAD_URL:-} ]] || die "set MAST_DOWNLOAD_URL in mast.env (where testers download from)"
command -v readelf >/dev/null || die "readelf not found (binutils)"
command -v uv >/dev/null || die "uv not found"
SIGN=1
[[ ${MAST_UNSIGNED:-} == 1 ]] && SIGN=""
if [[ -n $SIGN ]]; then
	command -v gpg >/dev/null || die "gpg not found (GnuPG signs the release)"
	gpg --list-secret-keys "$MAST_SIGNING_KEY" >/dev/null 2>&1 ||
		die "the signing key $MAST_SIGNING_KEY isn't in your GnuPG (MAST_UNSIGNED=1 for a test build)"
fi

# nfpm: one static binary from its GitHub release, checked against the release's checksums.
# (IPv4: on a network with broken IPv6, Nix and curl's defaults hang.)
NFPM=$(command -v nfpm || true)
if [[ -z $NFPM ]]; then
	NFPM=$TOOLS/nfpm-$NFPM_VERSION
	if [[ ! -x $NFPM ]]; then
		log "fetching nfpm $NFPM_VERSION"
		mkdir -p "$TOOLS"
		tmp=$(mktemp -d)
		base=https://github.com/goreleaser/nfpm/releases/download/v$NFPM_VERSION
		name=nfpm_${NFPM_VERSION}_Linux_x86_64.tar.gz
		curl -4 -fL --retry 3 -o "$tmp/$name" "$base/$name"
		curl -4 -fsSL --retry 3 -o "$tmp/checksums.txt" "$base/checksums.txt"
		(cd "$tmp" && grep "  $name\$" checksums.txt | sha256sum -c --quiet -) || die "nfpm download damaged"
		tar -xzf "$tmp/$name" -C "$tmp" nfpm
		mv "$tmp/nfpm" "$NFPM"
		rm -rf "$tmp"
	fi
fi

# Chromium version plus the time it's packaged, so every release (a new build, or the same
# build packaged again with a fix) upgrades the last one.
VERSION=$(cat "$ROOT/dist/VERSION")
RELEASE=$(date -u +%Y%m%d%H%M)
FULL=$VERSION-$RELEASE
TAG=v$FULL
log "packaging Mast $FULL"

# Stage: the browser, plus its launcher file and icons beside it (the tarball install
# reads them from there; Chrome ships its icons beside the browser the same way).
rm -rf "$STAGE" "$OUT"
mkdir -p "$STAGE/mast" "$STAGE/share" "$OUT"
cp -a "$SRC/." "$STAGE/mast/"
cp "$P/mast.desktop" "$STAGE/mast/"
for s in 24 48 64 128 256; do
	cp "$ICONS/product_logo_$s.png" "$STAGE/mast/"
done
# What this install is, for Mast's own update check (mast_update_check.cc): the tarball's
# copy is compared with latest.json once a day.
echo "$FULL" > "$STAGE/mast/mast-version"
SANDBOX=""
if [[ -e $STAGE/mast/chrome-sandbox ]]; then
	# Packaged separately so it can be given the setuid bit.
	mv "$STAGE/mast/chrome-sandbox" "$STAGE/share/chrome-sandbox"
	SANDBOX=1
else
	log "warning: this build has no chrome-sandbox (built before build.sh shipped it)."
	log "         Ubuntu 24.04+ / Mint 22 will still start Mast through the AppArmor profile."
fi

# The package sources the .deb and .rpm install (the same lines install.sh would write).
sed "s|@BASE@|${MAST_DOWNLOAD_URL%/}|g" "$P/mast.sources" > "$STAGE/share/mast.sources"
sed "s|@PAGES@|${MAST_PAGES_URL%/}|g" "$P/mast.repo" > "$STAGE/share/mast.repo"

# rpm: depend on the exact libraries the browser links (distro-neutral, works on Fedora
# and openSUSE alike). GTK is loaded at runtime, so it's added by hand.
mapfile -t RPM_DEPS < <(readelf -d "$SRC/chrome" | sed -n 's/.*Shared library: \[\(.*\)\]/\1/p' |
	grep -vE '^(libc|libm|libdl|libpthread|librt|libgcc_s|ld-linux-x86-64)\.so' |
	sed 's/$/()(64bit)/'; echo 'libgtk-3.so.0()(64bit)'; echo xdg-utils)

# deb: the packages behind those libraries. Debian 13 and Ubuntu 24.04+ renamed seven of
# them for 64-bit time ("t64") and don't provide the old names, so either name will do
# (checked on Ubuntu 24.04 and 26.04, Debian 13; Debian 12 and Ubuntu 22.04 have the old ones).
DEB_DEPS=("libasound2t64 | libasound2" "libatk-bridge2.0-0t64 | libatk-bridge2.0-0"
	"libatk1.0-0t64 | libatk1.0-0" "libatspi2.0-0t64 | libatspi2.0-0" libcairo2
	"libcups2t64 | libcups2" libdbus-1-3 libexpat1 libgbm1 "libglib2.0-0t64 | libglib2.0-0"
	"libgtk-3-0t64 | libgtk-3-0" libnspr4 libnss3 libpango-1.0-0 libudev1 libx11-6 libxcb1
	libxcomposite1 libxdamage1 libxext6 libxfixes3 libxkbcommon0 libxrandr2 xdg-utils)

# The RPM's own signature: nfpm signs with the key itself, so it's exported for these few
# seconds into memory-backed storage (your session's /run/user), then removed.
KEYFILE=""
if [[ -n $SIGN ]]; then
	KEYDIR=$(mktemp -d "${XDG_RUNTIME_DIR:-/tmp}/mast-sign.XXXXXX")
	trap 'rm -rf "$KEYDIR" "$STAGE"' EXIT
	KEYFILE=$KEYDIR/key.asc
	# (A passphrase piped in without a final newline still counts.)
	read -rsp "Passphrase for the release key $MAST_SIGNING_KEY: " NFPM_RPM_PASSPHRASE || [[ -n $NFPM_RPM_PASSPHRASE ]]
	echo
	export NFPM_RPM_PASSPHRASE
	gpg --batch --pinentry-mode loopback --passphrase-fd 3 --armor \
		--export-secret-keys "$MAST_SIGNING_KEY" > "$KEYFILE" 3<<<"$NFPM_RPM_PASSPHRASE" ||
		die "couldn't unlock the release key (wrong passphrase?)"
fi

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
  # Mast's package source and the key that signs it: updates arrive with the system's own.
  - src: $P/mast-release-key.asc
    dst: /usr/share/keyrings/mast.asc
    packager: deb
  - src: $STAGE/share/mast.sources
    dst: /etc/apt/sources.list.d/mast.sources
    type: config|noreplace
    packager: deb
  - src: $P/mast-release-key.asc
    dst: /etc/pki/rpm-gpg/RPM-GPG-KEY-mast
    packager: rpm
  - src: $STAGE/share/mast.repo
    dst: /etc/yum.repos.d/mast.repo
    type: config|noreplace
    packager: rpm
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
EOF
	[[ -n $KEYFILE ]] && printf '  signature:\n    key_file: %s\n' "$KEYFILE"
	cat <<EOF
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
"$NFPM" package -f "$STAGE/nfpm.yaml" -p deb -t "$OUT/mast_amd64.deb"
log "building the .rpm${SIGN:+ (signed)}"
"$NFPM" package -f "$STAGE/nfpm.yaml" -p rpm -t "$OUT/mast.x86_64.rpm"
[[ -n $KEYFILE ]] && rm -f "$KEYFILE"

log "building the tarball"
[[ -n $SANDBOX ]] && cp -p "$STAGE/share/chrome-sandbox" "$STAGE/mast/chrome-sandbox"
XZ_OPT=-T0 tar -C "$STAGE/mast" --owner=0 --group=0 -cJf "$OUT/mast-linux-x86_64.tar.xz" .

# The installer, with the public key inside it: it checks SHA256SUMS.asc against that, never
# against a key downloaded next to the files it checks.
awk -v base="${MAST_DOWNLOAD_URL%/}" -v keyfile="$P/mast-release-key.asc" '
	/@KEY@/ { while ((getline line < keyfile) > 0) print line; next }
	{ gsub(/@BASE@/, base); print }' "$P/install.sh" > "$OUT/install.sh"
chmod 755 "$OUT/install.sh"
cp "$P/mast-release-key.asc" "$OUT/mast-release-key.asc"
(cd "$OUT" && { echo "# version $FULL"; sha256sum mast_amd64.deb mast.x86_64.rpm mast-linux-x86_64.tar.xz install.sh; } > SHA256SUMS)

log "package sources"
uv run --quiet "$P/repo.py" "$OUT" "$FULL" \
	"https://github.com/$MAST_RELEASES_REPO/releases/download/$TAG"
[[ -f $OUT/rpm/repodata/repomd.xml ]] || die "the dnf source wasn't written"

if [[ -n $SIGN ]]; then
	log "signing"
	g() { gpg --batch --yes --pinentry-mode loopback --passphrase-fd 3 --local-user "$MAST_SIGNING_KEY" \
		--digest-algo SHA256 "$@" 3<<<"$NFPM_RPM_PASSPHRASE"; }
	g --clearsign -o "$OUT/InRelease" "$OUT/Release"
	g --armor --detach-sign -o "$OUT/Release.gpg" "$OUT/Release"
	g --armor --detach-sign -o "$OUT/rpm/repodata/repomd.xml.asc" "$OUT/rpm/repodata/repomd.xml"
	g --armor --detach-sign -o "$OUT/SHA256SUMS.asc" "$OUT/SHA256SUMS"
	for s in InRelease Release.gpg rpm/repodata/repomd.xml.asc SHA256SUMS.asc; do
		case $s in InRelease) gpg --verify "$OUT/$s" 2>/dev/null ;; *) gpg --verify "$OUT/$s" "$OUT/${s%.*}" 2>/dev/null ;; esac ||
			die "signature check failed: $s"
	done
fi
unset NFPM_RPM_PASSPHRASE
rm -rf "$STAGE"
echo "$TAG" > "$OUT/TAG"

log "done: dist/release/ ($TAG${SIGN:+, signed})"
ls -lh "$OUT" | tail -n +2
[[ -n $SIGN ]] && log "publish it with: ./mast publish"
log "testers run:  curl -fsSL ${MAST_DOWNLOAD_URL%/}/install.sh | sh"
