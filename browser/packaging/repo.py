# /// script
# requires-python = ">=3.11"
# dependencies = []
# ///
"""Package-source metadata for a release in dist/release (run by package.sh, through uv):

  apt   Packages, Packages.gz, Release: a flat repository served from the GitHub release
        itself (sources line: <download URL>/ with suite "/"). InRelease and Release.gpg
        are the signatures, made by package.sh with the release key.
  dnf   rpm/repodata/: served from GitHub Pages (release files can't sit in a folder). Each
        package's address is that release's own folder, so metadata never points at a file
        a newer release replaced. repomd.xml.asc is the signature (package.sh).
  Mast  latest.json: what tarball installs compare themselves against once a day.

  uv run packaging/repo.py <dist/release> <version> <rpm base URL>
"""
import email.utils
import gzip
import hashlib
import io
import json
import os
import sys
import tarfile
import time

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import rpmrepo  # packaging/rpmrepo.py: plain Python, so no GLib or librpm is needed

out, version, rpm_base = sys.argv[1:4]


def digest(path, algo):
    h = hashlib.new(algo)
    with open(path, "rb") as f:
        for block in iter(lambda: f.read(1 << 20), b""):
            h.update(block)
    return h.hexdigest()


def deb_control(path):
    """The control file of a .deb: an ar archive holding control.tar.{gz,xz,zst}."""
    with open(path, "rb") as f:
        assert f.read(8) == b"!<arch>\n", "not a .deb"
        while header := f.read(60):
            name = header[:16].decode().strip().rstrip("/")
            size = int(header[48:58].decode().strip())
            data = f.read(size)
            f.read(size % 2)  # members are 2-byte aligned
            if name.startswith("control.tar"):
                with tarfile.open(fileobj=io.BytesIO(data)) as t:
                    for m in t.getmembers():
                        if m.name in ("./control", "control"):
                            return t.extractfile(m).read().decode()
    raise SystemExit("no control file in " + path)


# apt ------------------------------------------------------------------------------------
deb = os.path.join(out, "mast_amd64.deb")
stanza = deb_control(deb).rstrip("\n") + "\n" + "".join([
    "Filename: mast_amd64.deb\n",
    f"Size: {os.path.getsize(deb)}\n",
    f"MD5sum: {digest(deb, 'md5')}\n",
    f"SHA256: {digest(deb, 'sha256')}\n",
])
packages = os.path.join(out, "Packages")
open(packages, "w").write(stanza + "\n")
with open(packages, "rb") as src, gzip.GzipFile(packages + ".gz", "wb", mtime=0) as dst:
    dst.write(src.read())
release = [
    "Origin: Mast",
    "Label: Mast",
    "Architectures: amd64",
    f"Date: {email.utils.formatdate(time.time(), usegmt=True)}",
    "Description: Mast, a deep-work browser",
    "SHA256:",
]
for name in ("Packages", "Packages.gz"):
    p = os.path.join(out, name)
    release.append(f" {digest(p, 'sha256')} {os.path.getsize(p):>10} {name}")
open(os.path.join(out, "Release"), "w").write("\n".join(release) + "\n")

# dnf ------------------------------------------------------------------------------------
rpmrepo.write_repodata(os.path.join(out, "rpm", "repodata"),
                       [(os.path.join(out, "mast.x86_64.rpm"), "mast.x86_64.rpm")],
                       rpm_base.rstrip("/") + "/")

# Mast's own check -------------------------------------------------------------------------
tarball = os.path.join(out, "mast-linux-x86_64.tar.xz")
json.dump({
    "version": version,
    "tarball": "mast-linux-x86_64.tar.xz",
    "sha256": digest(tarball, "sha256"),
}, open(os.path.join(out, "latest.json"), "w"), indent=1)
print(f"repo metadata for {version}: Packages, Release, rpm/repodata, latest.json")
