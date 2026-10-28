#!/usr/bin/env bash
# One-time builder setup (Ubuntu 24.04): packages, idle auto-stop, depot_tools, then the
# first Chromium sync + build via build.sh. Safe to re-run.
set -euxo pipefail
export DEBIAN_FRONTEND=noninteractive NEEDRESTART_MODE=a
M=$HOME/mast

sudo apt-get update
sudo -E apt-get install -y git python3 curl lsb-release zstd rsync

# Idle auto-stop
echo "${MAST_IDLE_MINUTES:-20}" | sudo tee /etc/mast-idle-minutes >/dev/null
sudo install -m 755 "$M/remote/idle-stop.sh" /usr/local/bin/mast-idle-stop
sudo install -m 644 "$M/remote/mast-idle-stop.service" "$M/remote/mast-idle-stop.timer" /etc/systemd/system/
sudo systemctl daemon-reload
sudo systemctl enable --now mast-idle-stop.timer

# depot_tools (gclient, gn, siso, autoninja)
[ -d "$HOME/depot_tools" ] || git clone --depth 1 https://chromium.googlesource.com/chromium/tools/depot_tools.git "$HOME/depot_tools"
grep -q depot_tools "$HOME/.profile" || echo 'export PATH=$HOME/depot_tools:$PATH' >> "$HOME/.profile"
export PATH=$HOME/depot_tools:$PATH

mkdir -p "$HOME/chromium"
cd "$HOME/chromium"
if [ ! -f .gclient ]; then
	cat > .gclient <<'GCLIENT'
solutions = [{
  "name": "src",
  "url": "https://chromium.googlesource.com/chromium/src.git",
  "managed": False,
  "custom_deps": {},
  "custom_vars": { "checkout_pgo_profiles": False },
}]
GCLIENT
fi

# First sync needs the source tree before build deps can be installed.
VERSION=$(cat "$M/chromium_version.txt")
if [ ! -d src/build ]; then
	gclient sync --nohooks --no-history --revision "src@refs/tags/$VERSION"
fi
sudo -E ./src/build/install-build-deps.sh --no-prompt

exec bash "$M/remote/build.sh"
