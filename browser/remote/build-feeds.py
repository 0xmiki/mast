#!/usr/bin/env python3
"""Fetch the large maintained blocklists Mast ships beside the browser, as plain data files.

    build-feeds.py <blocklist dir> <out dir> <cache dir>

Writes <out>/adult.txt and <out>/gambling.txt (one domain per line, sorted; each also covers
its subdomains), SOURCES.txt and the feeds' licence. The browser reads them at startup
(src/mast/browser/mast_feeds.cc):
  - adult: HaGeZi NSFW + blocklist/feeds/adult.txt;
  - gambling: HaGeZi gambling mini + blocklist/feeds/gambling.txt.
Both are always blocked, in every window, and aren't part of the blocklist.

Hand-curated entries win: a feed entry that is, or is a parent of, a domain in
blocklist.json or blocklist/feeds/allow.txt is dropped (e.g. okcupid.com is Dating, not
adult; sleeper.com is Sports). A failed download falls back to the last good copy in the
cache dir, so a flaky network never ships an empty list.
"""
import json
import os
import sys
import time
import urllib.request

FEEDS = {
    "adult": "https://raw.githubusercontent.com/hagezi/dns-blocklists/main/wildcard/nsfw-onlydomains.txt",
    "gambling": "https://raw.githubusercontent.com/hagezi/dns-blocklists/main/wildcard/gambling.mini-onlydomains.txt",
}
LICENSE = "https://raw.githubusercontent.com/hagezi/dns-blocklists/main/LICENSE"
MIN_ENTRIES = 10000  # anything smaller is a broken download, not a feed


def fetch(url, cache):
    for attempt in range(4):
        try:
            with urllib.request.urlopen(url, timeout=60) as r:
                text = r.read().decode("utf-8")
            with open(cache, "w", encoding="utf-8") as f:
                f.write(text)
            return text
        except Exception as e:  # noqa: BLE001 - any failure falls back to the cache
            print(f"  {url}: {e}", file=sys.stderr)
            time.sleep(2 * (attempt + 1))
    if os.path.exists(cache):
        print(f"  using cached {cache}", file=sys.stderr)
        return open(cache, encoding="utf-8").read()
    sys.exit(f"cannot fetch {url} and no cached copy")


def hosts(text):
    return [l.strip().lower() for l in text.splitlines() if l.strip() and not l.lstrip().startswith("#")]


def header(text, key):
    for l in text.splitlines():
        if l.startswith(f"# {key}:"):
            return l.split(":", 1)[1].strip()
    return "?"


def parents(domain):
    """example.co.uk -> {example.co.uk, co.uk} (not the bare TLD)."""
    parts = domain.split(".")
    return {".".join(parts[i:]) for i in range(len(parts) - 1)}


def under(domain, roots):
    return any(p in roots for p in parents(domain))


def main():
    blocklist, out, cache = sys.argv[1:4]
    os.makedirs(out, exist_ok=True)
    os.makedirs(cache, exist_ok=True)
    data = json.load(open(os.path.join(blocklist, "blocklist.json"), encoding="utf-8"))
    curated = set()
    for c in data["categories"]:
        for s in c["services"]:
            curated.update(s["domains"])
            curated.update(s.get("except", []))
    allow = set(hosts(open(os.path.join(blocklist, "feeds/allow.txt"), encoding="utf-8").read()))
    # Feed entries that would block a curated or allowed site: the site itself or a parent.
    protected = set()
    for d in curated | allow:
        protected |= parents(d)

    sources = ["Blocklists shipped with Mast. Each file: one domain per line, covering its subdomains.", ""]
    for name, url in FEEDS.items():
        text = fetch(url, os.path.join(cache, f"{name}.txt"))
        entries = set(hosts(text))
        if len(entries) < MIN_ENTRIES:
            sys.exit(f"{name}: only {len(entries)} entries from {url}")
        extra = os.path.join(blocklist, f"feeds/{name}.txt")  # Mast's hand-picked sites
        if os.path.exists(extra):
            entries |= set(hosts(open(extra, encoding="utf-8").read()))
        dropped = sorted(e for e in entries if e in protected or under(e, allow))
        entries -= set(dropped)
        # Subdomains of another entry are redundant: matching covers subdomains.
        entries = {e for e in entries if not any(p in entries for p in parents(e) - {e})}
        with open(os.path.join(out, f"{name}.txt"), "w", encoding="utf-8") as f:
            f.write("\n".join(sorted(entries)) + "\n")
        print(f"  {name}: {len(entries)} domains (HaGeZi {header(text, 'Version')}; "
              f"{len(dropped)} left to the curated list: {' '.join(dropped[:12])}{' …' if len(dropped) > 12 else ''})")
        sources += [f"{name}.txt: {len(entries)} domains",
                    f"  {url}",
                    f"  HaGeZi version {header(text, 'Version')}, licence GPL-3.0 (LICENSE-HaGeZi.txt)",
                    f"  minus {len(dropped)} domains Mast's own list places elsewhere"
                    + f", plus Mast's blocklist/feeds/{name}.txt",
                    ""]
    with open(os.path.join(out, "SOURCES.txt"), "w", encoding="utf-8") as f:
        f.write("\n".join(sources))
    with open(os.path.join(out, "LICENSE-HaGeZi.txt"), "w", encoding="utf-8") as f:
        f.write(fetch(LICENSE, os.path.join(cache, "LICENSE")))


if __name__ == "__main__":
    main()
