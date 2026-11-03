"""Embed Mast's HTML pages (and other data files) as C++ raw strings, written only when
they change.   embed-pages.py <src/mast/browser/resources> <out/gen/mast/browser/resources> [file...]

Each file becomes <name>_<ext>.inc: start.html -> start_html.inc, blocklist.json ->
blocklist_json.inc.
"""
import os
import sys

src, dst, *extra = sys.argv[1:]
os.makedirs(dst, exist_ok=True)
files = [os.path.join(src, n) for n in sorted(os.listdir(src)) if n.endswith(".html")] + extra
for path in files:
    name = os.path.basename(path)
    text = open(path, encoding="utf-8").read()
    assert ")MASTPAGE" not in text, name
    out = os.path.join(dst, name.replace(".", "_") + ".inc")
    inc = 'R"MASTPAGE(' + text + ')MASTPAGE"\n'
    if not os.path.exists(out) or open(out, encoding="utf-8").read() != inc:
        open(out, "w", encoding="utf-8").write(inc)
        print("embedded", name)
