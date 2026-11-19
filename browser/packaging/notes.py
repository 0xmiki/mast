# /// script
# requires-python = ">=3.11"
# dependencies = []
# ///
"""A release's notes on GitHub, laid out like Brave's: how to install, what's new (the
"Unreleased" list in CHANGELOG.md), which Chromium, how to check the files. Run by
publish.sh, through uv.

  uv run packaging/notes.py notes <CHANGELOG.md> <version> <ungoogled tag> <repo> [<previous tag>]
      -> prints the notes; fails when "Unreleased" lists nothing
  uv run packaging/notes.py file <CHANGELOG.md> <version>
      -> files the "Unreleased" list under the version; "Unreleased" starts empty
"""
import re
import sys

UNRELEASED = re.compile(r"^## Unreleased[ \t]*\n(.*?)(?=^## |\Z)", re.S | re.M)


def whats_new(changelog):
    m = UNRELEASED.search(open(changelog).read())
    return m.group(1).strip() if m else ""


def notes(changelog, version, ungoogled, repo, previous=""):
    new = whats_new(changelog)
    if not new:
        sys.exit('CHANGELOG.md has nothing under "## Unreleased": add what\'s new in this release first')
    chromium = version.rsplit("-", 1)[0]
    before = previous.removeprefix("v").rsplit("-", 1)[0] if previous else ""
    if before and before != chromium:
        log = f"https://chromium.googlesource.com/chromium/src/+log/{before}..{chromium}?pretty=fuller&n=10000"
        chromium_line = f"Upgraded Chromium to {chromium} ([changes since {before}]({log})), through ungoogled-chromium {ungoogled}."
    else:
        chromium_line = f"Chromium {chromium}, through ungoogled-chromium {ungoogled}."
    install = f"https://github.com/{repo}/releases/latest/download/install.sh"
    return f"""## Install

```sh
curl -fsSL {install} | sh
```

Or download the package for your system:

| System | File |
|---|---|
| Ubuntu, Debian, Mint, Pop!_OS | `mast_amd64.deb` |
| Fedora, RHEL, Rocky, AlmaLinux, openSUSE | `mast.x86_64.rpm` |
| Any distribution, no root needed | `mast-linux-x86_64.tar.xz` |

The .deb and .rpm add Mast's package source, so updates come with your system's updates.
`Packages`, `Release`, `InRelease` and the other files are that source: you don't need them.

## What's new

{new}

## Chromium

{chromium_line}

## Verify the download

`SHA256SUMS` lists every file's checksum, signed with Mast's release key (`F871BCDC5C591AFB`, in `mast-release-key.asc`):

```sh
gpg --import mast-release-key.asc && gpg --verify SHA256SUMS.asc SHA256SUMS && sha256sum -c --ignore-missing SHA256SUMS
```
"""


def file_under(changelog, version):
    text = open(changelog).read()
    new = whats_new(changelog)
    text = UNRELEASED.sub(lambda m: f"## Unreleased\n\n## {version}\n\n{new}\n\n", text, count=1)
    open(changelog, "w").write(text)


if __name__ == "__main__":
    cmd, *args = sys.argv[1:]
    if cmd == "notes":
        print(notes(*args), end="")
    elif cmd == "file":
        file_under(*args)
    else:
        sys.exit(__doc__)
