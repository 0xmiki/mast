"""Summarise where Mast talked to, from a net log, and flag anything that isn't a page we opened.

  netcheck.py <netlog.json> [chrome.log]
"""
import collections
import re
import sys

# Pages the screenshot scenes open on purpose (and what they load).
OPENED = ("wikipedia.org", "wikimedia.org", "youtube.com", "x.com")

text = open(sys.argv[1], errors="replace").read()
hosts = collections.Counter(re.findall(r'"(?:https?|wss?)://([^/":?]+)', text))
print("Hosts contacted (requests):")
for host, n in hosts.most_common():
    mark = "  " if host.endswith(OPENED) else "!!"
    print(f"  {mark} {n:5d}  {host}")
unexpected = [h for h in hosts if not h.endswith(OPENED)]
print(f"\n{len(unexpected)} host(s) not opened by the scenes" + (": " + ", ".join(sorted(unexpected)) if unexpected else ""))
if len(sys.argv) > 2:
    blocked = [l.strip() for l in open(sys.argv[2], errors="replace") if "qjz9zk" in l or "TrkProtocolHandler" in l]
    print(f"{len(blocked)} request(s) stopped by domain substitution (each is a feature that tried to reach Google)")
    for line in blocked[:30]:
        print("   ", line[:200])
