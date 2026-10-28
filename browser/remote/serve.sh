#!/usr/bin/env bash
# Publish ~/mast/dist through a temporary Cloudflare quick tunnel (no account),
# under an unguessable path. Hetzner -> some networks is very slow over plain SSH;
# Cloudflare's edge is close to almost everyone.
#   serve.sh <token>   start, print the https://….trycloudflare.com base URL
#   serve.sh stop      tear everything down
set -euo pipefail
if [ "${1:-}" = stop ]; then
	pkill -f "[c]loudflared tunnel" || true
	pkill -f "[h]ttp.server 8765" || true
	rm -rf /root/serve
	exit 0
fi
token=${1:?usage: serve.sh <token>|stop}
if [ ! -x /usr/local/bin/cloudflared ]; then
	curl -fsSL -o /usr/local/bin/cloudflared \
		https://github.com/cloudflare/cloudflared/releases/latest/download/cloudflared-linux-amd64
	chmod +x /usr/local/bin/cloudflared
fi
"$0" stop
mkdir -p "/root/serve/$token"
ln -s /root/mast/dist/mast-linux.tar.zst /root/mast/dist/VERSION "/root/serve/$token/"
sha256sum /root/mast/dist/mast-linux.tar.zst | cut -d' ' -f1 > "/root/serve/$token/sha256"
(cd /root/serve && nohup python3 -m http.server 8765 --bind 127.0.0.1 > /root/serve.log 2>&1 &)
(nohup cloudflared tunnel --no-autoupdate --url http://127.0.0.1:8765 > /root/cf.log 2>&1 &)
for _ in $(seq 45); do
	url=$(grep -oE 'https://[a-z0-9-]+\.trycloudflare\.com' /root/cf.log | head -1 || true)
	if [ -n "$url" ]; then echo "$url"; exit 0; fi
	sleep 2
done
echo "tunnel did not come up" >&2
exit 1
