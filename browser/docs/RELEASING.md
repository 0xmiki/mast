# Releasing Mast (Linux)

Three commands, after a build is downloaded (`./mast build` puts it in `dist/mast`):

```
./mast package    # dist/release: .deb, .rpm, tarball, installer, apt/dnf sources; signed
./mast publish    # GitHub release + GitHub Pages; apt and dnf users update from there
```

`package` asks for the release key's passphrase once. The key is `MAST_SIGNING_KEY` in
`mast.env` (F871BCDC5C591AFB, "Mast Releases"). It never leaves your GnuPG: the RPM's
signature needs the key as a file, so it's exported for a few seconds into `/run/user`
(memory, not disk) and removed. `MAST_UNSIGNED=1 ./mast package` makes a local test build,
which `publish` refuses.

## How each install gets updates

| Install | How it updates | What checks it |
|---|---|---|
| `.deb` (Ubuntu, Debian, Mint, Pop!_OS) | With the system's updates. The package installs `/etc/apt/sources.list.d/mast.sources` and the key in `/usr/share/keyrings/mast.asc` | apt verifies `InRelease` against the key, then each file's SHA256 |
| `.rpm` (Fedora, openSUSE) | With the system's updates. The package installs `/etc/yum.repos.d/mast.repo` (dnf) and `/etc/zypp/repos.d/mast.repo` (zypper), and imports the key into rpm | Every RPM is signed; zypper also checks the signed metadata |
| Tarball (`~/.local/opt/mast`) | Mast checks `latest.json` once a day and says when a release is newer, with the update command to copy (install.sh again) | install.sh verifies `SHA256SUMS.asc` with the key built into it (when GnuPG is installed), then each file's SHA256 |

## Where things are served from

- **The GitHub release** (`MAST_RELEASES_REPO`: this repo, public): the packages, the installer,
  `SHA256SUMS(.asc)`, `latest.json`, and the apt source itself: a flat repository
  (`Packages`, `Release`, `InRelease`) read through `releases/latest/download/`. A release
  is uploaded as a draft and made "latest" only once complete, so its files switch together.
- **GitHub Pages** (`MAST_PAGES_URL`, the repo's `gh-pages` branch, which holds only this): the dnf/zypper metadata (`rpm/repodata`), which
  can't be a release file because it lives in a folder. It points at the RPM in its own
  release's folder (`releases/download/<tag>/`), never at "latest", so old metadata never
  names a newer file. Also a copy of the key and the installer.

## The key

- Back up the private key and its revocation certificate offline (password manager or a
  USB stick). Lose the key, and every install has to be told to trust a new one.
- It expires on 2029-11-17. Extend it before then: `gpg --quick-set-expire F871BCDC5C591AFB 3y`,
  then export the public key again into `packaging/mast-release-key.asc` and release.
- If it's ever stolen: publish the revocation certificate, make a new key, and ship a
  release (signed with the old key, while it still works) that installs the new one.

## Known limits

- Older rpm releases (openSUSE Leap 15, RHEL/Alma/Rocky 8) may not verify ed25519 signatures.
  Fedora and openSUSE Tumbleweed do.
- dnf doesn't check the repository metadata's signature (`repo_gpgcheck=0`, like Google
  Chrome's source): dnf keeps those keys apart from rpm's and would stop to ask about the key,
  which background updaters can't answer. Every package it installs is still signature-checked.
