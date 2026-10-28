"""Embed Mast's HTML pages as C++ raw strings (written only when they change).

  embed-pages.py <src/mast/browser/resources> <out/gen/mast/browser/resources>
"""
import os
import sys

src, dst = sys.argv[1], sys.argv[2]
os.makedirs(dst, exist_ok=True)
for name in sorted(os.listdir(src)):
    if not name.endswith(".html"):
        continue
    html = open(os.path.join(src, name), encoding="utf-8").read()
    assert ")MASTPAGE" not in html, name
    out = os.path.join(dst, name.replace(".html", "_html.inc"))
    text = 'R"MASTPAGE(' + html + ')MASTPAGE"\n'
    if not os.path.exists(out) or open(out, encoding="utf-8").read() != text:
        open(out, "w", encoding="utf-8").write(text)
        print("embedded", name)
