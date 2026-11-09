#!/bin/sh
# Runs as root after the .deb or .rpm removes Mast. On upgrades (deb: "upgrade", rpm: 1)
# the new package's postinst has already rewritten the profile, so leave it alone.
case "${1:-}" in
remove | purge | 0)
	if [ -f /etc/apparmor.d/mast ]; then
		if command -v apparmor_parser >/dev/null 2>&1 && [ -d /sys/kernel/security/apparmor ]; then
			apparmor_parser -R /etc/apparmor.d/mast 2>/dev/null || true
		fi
		rm -f /etc/apparmor.d/mast
	fi
	;;
esac
exit 0
