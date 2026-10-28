#!/usr/bin/env bash
# Runs every few minutes on the builder (systemd timer). Shuts the machine down — which
# *stops* the EC2 instance — once nothing has been running for /etc/mast-idle-minutes.
set -u
IDLE_MIN=$(cat /etc/mast-idle-minutes 2>/dev/null || echo 20)
STAMP=/run/mast-active   # tmpfs: recreated on every boot

busy=0
pgrep -f 'mast/remote/(bootstrap|build)\.sh' >/dev/null && busy=1
pgrep -x 'siso|ninja|gclient' >/dev/null && busy=1
[ -n "$(ss -Htn state established '( sport = :22 )')" ] && busy=1

if [ "$busy" = 1 ] || [ ! -f "$STAMP" ]; then
	touch "$STAMP"
	exit 0
fi

idle=$(( ($(date +%s) - $(stat -c %Y "$STAMP")) / 60 ))
if [ "$idle" -ge "$IDLE_MIN" ]; then
	logger -t mast "idle for ${idle}m, shutting down"
	shutdown -h now
fi
