"""Keep build timestamps of files whose content didn't change across a patch re-apply.

  keep-mtimes.py save <src> <state>     before `git reset --hard`: record every modified file
  keep-mtimes.py restore <src> <state>  after patches, overrides and renames: put back the old
                                        mtime wherever the content is byte-identical

Without this, changing one Mast patch would touch every patched file (including widely
included headers) and recompile thousands of objects for nothing.
"""
import hashlib
import json
import os
import subprocess
import sys

mode, src, state = sys.argv[1:]


def digest(path):
    with open(path, "rb") as f:
        return hashlib.sha256(f.read()).hexdigest()


if mode == "save":
    names = subprocess.run(["git", "-C", src, "diff", "--name-only", "HEAD"],
                           capture_output=True, text=True, check=True).stdout.split()
    saved = {}
    for name in names:
        path = os.path.join(src, name)
        if os.path.isfile(path):
            saved[name] = [digest(path), os.stat(path).st_mtime_ns]
    json.dump(saved, open(state, "w"))
    print(f"  remembered {len(saved)} modified files")
else:
    if not os.path.exists(state):
        sys.exit(0)
    kept = 0
    for name, (sha, mtime) in json.load(open(state)).items():
        path = os.path.join(src, name)
        if os.path.isfile(path) and digest(path) == sha:
            os.utime(path, ns=(mtime, mtime))
            kept += 1
    os.remove(state)
    print(f"  kept the timestamps of {kept} unchanged files")
