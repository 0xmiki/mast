#!/bin/sh
# Runs as root after the .deb or .rpm installs or upgrades Mast.
set -e

# The sandbox helper must be root-owned and setuid (some tools drop the bit on unpack).
if [ -e /opt/mast/chrome-sandbox ]; then
	chown root:root /opt/mast/chrome-sandbox
	chmod 4755 /opt/mast/chrome-sandbox
fi

# AppArmor 4 (Ubuntu 24.04+, Linux Mint 22) restricts user namespaces, which Chromium's
# sandbox uses. Allow them for Mast, the way Ubuntu does for Chrome. Written only on
# AppArmor 4: older parsers reject the abi line.
if [ -f /etc/apparmor.d/abi/4.0 ]; then
	cat > /etc/apparmor.d/mast <<'EOF'
# Installed by the mast package: lets Mast's sandbox use user namespaces.
abi <abi/4.0>,
include <tunables/global>

profile mast /opt/mast/chrome flags=(unconfined) {
  userns,

  include if exists <local/mast>
}
EOF
	if command -v apparmor_parser >/dev/null 2>&1 && [ -d /sys/kernel/security/apparmor ]; then
		apparmor_parser -r -T -W /etc/apparmor.d/mast || true
	fi
fi

# The .rpm's key isn't imported here: rpm holds its own lock while it runs this, so it would
# fail. install.sh imports it before installing; a hand-installed .rpm gets it from the
# gpgkey= line in mast.repo the first time dnf or zypper updates (after one confirmation).

command -v update-desktop-database >/dev/null 2>&1 && update-desktop-database -q /usr/share/applications || true
command -v gtk-update-icon-cache >/dev/null 2>&1 && gtk-update-icon-cache -q -t /usr/share/icons/hicolor || true
exit 0
