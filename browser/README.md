# Mast browser (Chromium fork)

Linux only for now. Chromium is built on an AWS machine that **only runs while building**:
each build starts it, builds incrementally on its kept disk, downloads the browser, and
stops it. A timer on the machine also shuts it down after 20 idle minutes, in case your
laptop disconnects mid-build. UI prototype and design docs live in `../browser` (see its
`docs/CHROMIUM.md`).

## One-time setup

```sh
nix develop            # aws, rsync, ssh, zstd
aws configure          # your AWS access key, region us-east-1
./mast provision       # creates the builder, syncs Chromium, first full build (a few hours)
./mast resize small    # drop to a cheaper machine for incremental builds
```

New AWS accounts often allow only a few vCPUs. Before provisioning, request a quota
increase for "Running On-Demand Standard instances" (192 vCPUs for the first build;
64 is enough afterwards), or set `MAST_BIG_TYPE` in `mast.env` to something smaller.

## Every build

```sh
./mast build           # start → push src/mast + patches → build → download → stop
./mast run             # run it locally (uses an FHS shell on NixOS)
```

## Costs (us-east-1, on-demand)

| | |
|---|---|
| Stopped builder (300 GB disk) | ≈ $29 / month |
| First full build on `c7a.48xlarge` | ≈ $10–20 (estimate) |
| Incremental build on `c7a.16xlarge` | ≈ $3.28 / hour while running |
| `./mast destroy` | $0 afterwards (deletes the disk too) |

## Layout

- `chromium_version.txt` — pinned Chromium stable version
- `src/mast/` — Mast's own Chromium code (copied to `chromium/src/mast/`)
- `patches/series` — small hook patches applied to Chromium, in order
- `remote/` — scripts that run on the builder (bootstrap, build, idle auto-stop)
- `mast` / `mast.env` — the CLI and its settings
