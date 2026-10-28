"""Rename the product to Mast in every UI string file (.grd/.grdp) of the tree.

Runs on the builder during each build (after patches). Only files whose text
changes are rewritten, so unchanged files keep their mtimes and don't rebuild.
English only for now: translated strings whose English source changed fall back
to English until we add .xtb fingerprint remapping.

  substitute-names.py <chromium/src> [--dry-run]
"""
import os
import re
import sys

SRC = sys.argv[1]
DRY = "--dry-run" in sys.argv
ROOTS = ("chrome", "components", "ui")
SKIP = re.compile(r"/(chromeos|ash|remoting|android|ios|fuchsia|test|tests|testdata|"
                  r"webview|cast_receiver|lacros)(/|$)")

# Order matters. Credits to Chromium (required by its licence) are protected.
PROTECT = [
    "Chromium open source project", "Chromium project",
    "Chrome Web Store", "Chrome Remote Desktop", "Chrome OS", "ChromeOS", "Chromebook",
    "Chromecast", "Chrome Enterprise", "Chrome Browser Cloud", "Chrome Sync",
    "Google Chrome Frame",
]
RULES = [
    (re.compile(r"The Chromium Authors"), "The Mast Authors"),
    (re.compile(r"\bGoogle Chrome\b"), "Mast"),
    (re.compile(r"\bChromium\b"), "Mast"),
    (re.compile(r"\bChrome\b(?![.:/_-])"), "Mast"),
]


def rename(text):
    # Copyright first, then shield protected phrases, then rename, then unshield.
    text = RULES[0][0].sub(RULES[0][1], text)
    shields = {}
    # The credit link "…made possible by the <link>Chromium</link> open source
    # project" must keep naming Chromium (its licence asks for attribution).
    text = re.sub(r'Chromium(?=<ph name="END_LINK_CHROMIUM")', "\x00credit\x00", text)
    shields["\x00credit\x00"] = "Chromium"
    for i, phrase in enumerate(PROTECT):
        token = f"\x00{i}\x00"
        if phrase in text:
            shields[token] = phrase
            text = text.replace(phrase, token)
    # Only rename inside message bodies and descriptions, never in attributes such
    # as name="IDS_..."/expr="..."/file="..." (those are upper/lower case anyway).
    def body(m):
        s = m.group(0)
        for rx, rep in RULES[1:]:
            s = rx.sub(rep, s)
        return s
    text = re.sub(r"<message\b[\s\S]*?</message>", body, text)
    for token, phrase in shields.items():
        text = text.replace(token, phrase)
    return text


changed = files = 0
for root in ROOTS:
    for dirpath, dirnames, filenames in os.walk(os.path.join(SRC, root)):
        rel = "/" + os.path.relpath(dirpath, SRC)
        if SKIP.search(rel):
            dirnames[:] = []
            continue
        for name in filenames:
            if not name.endswith((".grd", ".grdp")):
                continue
            path = os.path.join(dirpath, name)
            with open(path, encoding="utf-8") as f:
                old = f.read()
            new = rename(old)
            files += 1
            if new != old:
                changed += 1
                if DRY:
                    a = len(re.findall(r"\b(?:Chromium|Chrome)\b", old))
                    b = len(re.findall(r"\b(?:Chromium|Chrome)\b", new))
                    print(f"{os.path.relpath(path, SRC)}: {a - b} renamed")
                else:
                    with open(path, "w", encoding="utf-8") as f:
                        f.write(new)
print(f"{'would change' if DRY else 'changed'} {changed} of {files} string files", file=sys.stderr)
