"""Merge GN args files; later files win.   merge-args.py <out> <in1> [in2 ...]

Used to put ungoogled-chromium's flags.gn under Mast's own args.gn.
"""
import re
import sys

out, *inputs = sys.argv[1:]
merged, order = {}, []
for path in inputs:
    for line in open(path):
        m = re.match(r"\s*([A-Za-z0-9_]+)\s*=\s*(.+?)\s*$", line)
        if m and not line.lstrip().startswith("#"):
            if m[1] not in merged:
                order.append(m[1])
            merged[m[1]] = m[2]
text = "".join(f"{k} = {merged[k]}\n" for k in order)
try:
    same = open(out).read() == text
except FileNotFoundError:
    same = False
if not same:
    open(out, "w").write(text)
    print("args.gn updated")
