#!/usr/bin/env bash
# Screenshot Mast's UI on the builder under Xvfb, for design review. Nothing needs a real
# screen and only small JPEGs travel back. Scenes live in shots.py.
set -euo pipefail
OUT=/root/chromium/src/out/Mast
SHOTS=/root/mast/shots
W=${MAST_SHOT_W:-1440}
H=${MAST_SHOT_H:-900}
if ! command -v Xvfb >/dev/null || ! command -v xdotool >/dev/null || ! command -v convert >/dev/null ||
	! python3 -c 'import websocket' 2>/dev/null; then
	DEBIAN_FRONTEND=noninteractive apt-get install -y -q xvfb xdotool imagemagick python3-websocket \
		fonts-inter fonts-noto-core >/dev/null
fi
pkill -f "[X]vfb :99" || true
rm -rf "$SHOTS" /tmp/mast-shot-profile
mkdir -p "$SHOTS"
Xvfb :99 -screen 0 "${W}x${H}x24" -nolisten tcp >/dev/null 2>&1 &
XVFB=$!
export DISPLAY=:99
sleep 1
# --no-sandbox only because the builder runs as root; never in a real install.
"$OUT/chrome" --no-sandbox --user-data-dir=/tmp/mast-shot-profile --no-first-run \
	--no-default-browser-check --test-type --ozone-platform=x11 --window-position=0,0 \
	--window-size="$W,$H" --remote-debugging-port=9333 --remote-allow-origins='*' \
	> "$SHOTS/chrome.log" 2>&1 &
CHROME=$!
trap 'kill $CHROME $XVFB 2>/dev/null || true' EXIT
python3 /root/mast/remote/shots.py "$SHOTS"
cd "$SHOTS"
for f in *.png; do convert "$f" -quality 85 "${f%.png}.jpg" && rm "$f"; done
ls
