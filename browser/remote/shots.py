"""Drive Mast through fixed scenes and screenshot the whole (virtual) screen.

CDP navigates pages; xdotool sends real key presses for native UI (omnibox, app menu).
UI DevTools (--enable-ui-devtools=9223) is the inspector for the native UI: each dump
records every view's class, screen bounds and properties (with Background/Border colour
ids) next to the screenshot, and finds native controls to click by their properties.
"""
import json
import subprocess
import sys
import time
import urllib.request

import websocket

OUT = sys.argv[1]
BASE = "http://127.0.0.1:9333"


def get(path, method="GET"):
    req = urllib.request.Request(BASE + path, method=method)
    return json.load(urllib.request.urlopen(req, timeout=10))


class Page:
    def __init__(self):
        target = next(t for t in get("/json") if t["type"] == "page")
        self.ws = websocket.create_connection(target["webSocketDebuggerUrl"], suppress_origin=True)
        self.id = 0

    def send(self, method, **params):
        self.id += 1
        self.ws.send(json.dumps({"id": self.id, "method": method, "params": params}))
        while True:
            msg = json.loads(self.ws.recv())
            if msg.get("id") == self.id:
                return msg

    def go(self, url, wait=3.0):
        self.send("Page.navigate", url=url)
        time.sleep(wait)

    def js(self, expression):
        return self.send("Runtime.evaluate", expression=expression, returnByValue=True)


class NativeUI:
    """Chromium's UI DevTools: the views tree as a DOM, properties as CSS (one client)."""

    def __init__(self, port=9223):
        self.ws = websocket.create_connection(f"ws://127.0.0.1:{port}/0", suppress_origin=True,
                                              timeout=20)
        self.id = 0
        self.send("DOM.enable")
        self.send("CSS.enable")

    def send(self, method, **params):
        self.id += 1
        self.ws.send(json.dumps({"id": self.id, "method": method, "params": params}))
        while True:
            msg = json.loads(self.ws.recv())
            if msg.get("id") == self.id:
                return msg.get("result", {})

    def tree(self):
        """Every node: id, type, class, name, depth, screen bounds, properties."""
        out = []

        def walk(node, depth):
            a = node.get("attributes", [])
            attrs = dict(zip(a[::2], a[1::2]))
            row = {"id": node["nodeId"], "type": node.get("nodeName"), "class": attrs.get("class"),
                   "name": attrs.get("name"), "depth": depth}
            if node.get("nodeName") in ("View", "Widget", "Window"):
                b = self.send("DOM.getNodeBoundsInScreen", nodeId=node["nodeId"])
                row["bounds"] = b.get("boundsInScreen") or b
                props = {}
                for m in self.send("CSS.getMatchedStylesForNode", nodeId=node["nodeId"]).get(
                        "matchedCSSRules", []):
                    rule = m.get("rule", {})
                    sel = (rule.get("selectorList", {}).get("selectors") or [{}])[0].get("text", "?")
                    props[sel] = {p["name"]: p.get("value") for p in rule.get("style", {}).get(
                        "cssProperties", [])}
                row["props"] = props
            out.append(row)
            for c in node.get("children", []):
                walk(c, depth + 1)

        walk(self.send("DOM.getDocument")["root"], 0)
        return out

    def dump(self, name):
        rows = self.tree()
        with open(f"{OUT}/views-{name}.json", "w") as f:
            json.dump(rows, f, indent=1)
        print("views", name, len(rows), flush=True)
        return rows

    def find(self, rows, cls=None, prop_contains=None):
        for r in rows:
            if cls and r.get("class") != cls:
                continue
            if prop_contains and prop_contains not in json.dumps(r.get("props", {})):
                continue
            if r.get("bounds"):
                return r
        return None

    def close(self):
        self.ws.close()


def native_dump(name, click=None):
    """Dump the views tree; optionally click the first view matching click=(class, text)."""
    try:
        ui = NativeUI()
    except Exception as e:  # never lose the screenshots over the inspector
        print("ui-devtools unavailable:", e, flush=True)
        return None
    try:
        rows = ui.dump(name)
        if click:
            r = ui.find(rows, *click)
            if not r:
                print("not found:", click, flush=True)
                return None
            b = r["bounds"]
            x, y = int(b["x"] + b["width"] / 2), int(b["y"] + b["height"] / 2)
            subprocess.run(["xdotool", "mousemove", str(x), str(y), "click", "1"])
            print("clicked", r.get("class"), x, y, flush=True)
            return r
        return rows
    except Exception as e:
        print("ui-devtools error:", e, flush=True)
        return None
    finally:
        ui.close()


def window():
    for name in ("Mast", "Chromium", "Chrome"):
        r = subprocess.run(["xdotool", "search", "--onlyvisible", "--name", name],
                           capture_output=True, text=True)
        if r.stdout.strip():
            return r.stdout.split()[0]
    return None


def keys(*seq, text=None):
    w = window()
    if w:
        subprocess.run(["xdotool", "windowfocus", "--sync", w])
    for k in seq:
        subprocess.run(["xdotool", "key", "--clearmodifiers", k])
        time.sleep(0.4)
    if text:
        subprocess.run(["xdotool", "type", "--delay", "60", text])


def shot(name, settle=1.5):
    time.sleep(settle)
    subprocess.run(["import", "-window", "root", f"{OUT}/{name}.png"], check=True)
    print("shot", name, flush=True)


for _ in range(80):
    try:
        get("/json/version")
        break
    except Exception:
        time.sleep(0.5)
time.sleep(4)

p = Page()
# What a new tab's icon shows while it opens (a Chromium logo was seen for a moment):
# the tab row, every 60ms for about a second after Ctrl+T.
keys("ctrl+t")
for i in range(16):
    subprocess.run(["import", "-window", "root", "-crop", "720x41+0+0", f"{OUT}/00-newtab-icon-{i:02d}.png"])
    time.sleep(0.06)
print("shot 00-newtab-icon frames", flush=True)
keys("ctrl+w")
# The new tab page: an empty desktop first, as a fresh profile sees it.
shot("01-desktop-empty")
native_dump("01-desktop-empty")
p.go("https://en.wikipedia.org/wiki/Ulysses_pact", 5)
shot("02-site")
keys("ctrl+l", text="ulysses")
shot("03-omnibox-typing")
keys("Escape", "Escape")
keys("alt+f")
shot("04-app-menu")
keys("Escape")
p.go("chrome://settings")
shot("05-settings")
p.go("chrome://settings/help")
shot("06-about")
p.go("chrome://version")
shot("07-version")
# The desktop with sites (Wikipedia has a favicon from the visit above; the rest fall back
# to a monogram until visited).
p.go("chrome://newtab")
for url, title in [("https://en.wikipedia.org/", "Wikipedia"), ("https://github.com/", "GitHub"),
                   ("https://svelte.dev/docs", "Svelte"), ("https://developer.mozilla.org/", "MDN"),
                   ("https://www.vitsoe.com/", "Vitsœ"), ("https://rsms.me/inter/", "Inter")]:
    p.js(f"chrome.send('desktopAdd', [{json.dumps(url)}, {json.dumps(title)}])")
    time.sleep(0.3)
p.go("chrome://newtab")
shot("08-desktop")
p.js("document.getElementById('edit').click()")
shot("09-distractions-change")
# The full blocklist (chrome://mast, "Edit blocklist").
p.go("chrome://mast")
p.js("document.querySelector('[data-toggle=shopping]').click()")
shot("10-blocklist")
# Block distractions for today, and one site forever.
p.go("chrome://newtab")
p.js("chrome.send('blockDistractions', ['today'])")
time.sleep(0.5)
p.js("chrome.send('block', [['x.com', 'twitter.com'], 'forever'])")
p.go("chrome://newtab")
shot("11-desktop-blocked")
p.go("https://www.youtube.com/", 4)
shot("12-blocked-timed")
p.go("https://x.com/", 4)
shot("13-blocked-forever")
# The always-on lists (mast_feeds.cc): a betting site only the gambling feed lists, and an
# adult site. Both always blocked, quietly (no end time).
p.go("https://www.pokerstars.net/", 4)
shot("30-gambling-feed")
p.go("https://onlyfans.com/", 4)
shot("31-quiet-block")
# Page info -> "Block this site" -> the block bubble -> undo.
p.go("https://en.wikipedia.org/wiki/Dieter_Rams", 5)
native_dump("14-site", click=("LocationIconView", None))
shot("14-page-info")
if native_dump("15-page-info", click=(None, "Block this site for a while")):
    shot("15-block-bubble")
    native_dump("15-block-bubble")
    keys("Return")
    shot("16-undo", settle=0.8)
keys("Escape")
# "Edit blocklist" opens the list in a panel on the new tab page.
p.go("chrome://newtab")
p.js("document.getElementById('editList').click()")
shot("17-blocklist-panel")
p.js("document.querySelector('.bl .always').scrollIntoView({block: 'center'})")
shot("32-always-blocked-note")
p.js("document.getElementById('blClose').click()")
# Customize: the panel, then a palette (the whole window should follow), then an image.
p.js("document.getElementById('customize').click()")
shot("18-customize")
p.js("chrome.send('setPalette', ['ink'])")
shot("19-palette-ink", settle=2)
native_dump("19-palette-ink")
p.js("""(() => {
  const c = document.createElement('canvas'); c.width = 1920; c.height = 1080;
  const g = c.getContext('2d'); const l = g.createLinearGradient(0, 0, 1920, 1080);
  l.addColorStop(0, '#1d3b53'); l.addColorStop(.55, '#7a4b3a'); l.addColorStop(1, '#e0a96d');
  g.fillStyle = l; g.fillRect(0, 0, 1920, 1080);
  g.fillStyle = 'rgba(255,255,255,.12)'; for (let i = 0; i < 9; i++) { g.beginPath(); g.arc(200 + i * 210, 760 - i * 40, 90 + i * 12, 0, 7); g.fill(); }
  const u = c.toDataURL('image/jpeg', .86); chrome.send('setWallpaper', [u.slice(u.indexOf(',') + 1)]);
})()""")
time.sleep(1.5)
p.go("chrome://newtab")
shot("20-wallpaper")
p.go("https://en.wikipedia.org/wiki/Dieter_Rams", 5)
shot("21-site-ink")
# mast:// works in the address bar and chrome:// pages show as mast://.
keys("ctrl+l", text="mast://version")
keys("Return")
shot("22-mast-scheme", settle=2.5)
# Settings follows the palette too (its page and cards).
p.go("chrome://settings/appearance", 4)
shot("23-settings-ink")
# A light picture: dark text and icons, light pills and tiles.
p.go("chrome://newtab")
p.js("chrome.send('setWallpaperTone', ['light'])")
time.sleep(1)
p.go("chrome://newtab")
shot("24-wallpaper-light", settle=2)
# Tab bar on websites (design C23): the Customize section, then each choice on websites.
# "Theme": the palette on a site, the picture stays on the new tab. "Match site": a light
# site (light row, dark text, a light address field and suggestions), a dark one, and one
# more (not Wikipedia: scene 15 may have blocked it); the colour comes from the top of the page a moment after it paints.
p.js("chrome.send('setWallpaperTone', ['dark'])")
time.sleep(1)
p.go("chrome://newtab")
p.js("document.getElementById('customize').click()")
shot("33-customize-tab-bar")
p.js("chrome.send('setTabBar', ['theme'])")
time.sleep(0.5)
p.go("https://example.com/", 4)
shot("34-theme-site", settle=1.5)
p.go("chrome://newtab")
p.js("chrome.send('setTabBar', ['site'])")
time.sleep(0.5)
p.go("https://example.com/", 4)
shot("35-site-example", settle=3)
native_dump("35-site-example")
keys("ctrl+l", text="example")
shot("36-site-omnibox", settle=1.5)
keys("Escape", "Escape")
p.go("https://github.com/", 6)
shot("37-site-github", settle=3)
p.go("https://developer.mozilla.org/en-US/", 6)
shot("38-site-mdn", settle=3)
p.go("https://www.python.org/", 6)
shot("39-site-python", settle=3)
p.go("chrome://newtab")
p.js("chrome.send('setTabBar', ['picture'])")
time.sleep(0.5)
# The star: Ctrl+D on a site saves it to the new tab (no folder picker), and a starred
# page shows a filled star.
p.go("https://www.vitsoe.com/", 5)
keys("ctrl+d")
shot("25-star-bubble", settle=1.5)
keys("Escape")
shot("26-starred", settle=1)
# The Web Store: the extensions page's link, and a listing that should offer to install.
p.go("chrome://extensions", 3)
shot("27-extensions")
p.go("https://chromewebstore.google.com/detail/ddkjiahejlhfcafbddmgiahcphecmpfh", 7)
shot("28-webstore-item", settle=2)
# Tab groups (design C22 A): the first tab grouped from its context menu and named "hi";
# its header pill and lane, next to an ungrouped tab.
p.go("chrome://newtab")
keys("ctrl+t")
time.sleep(1)
subprocess.run(["xdotool", "mousemove", "120", "20", "click", "3"])
time.sleep(1)
native_dump("29-tab-menu", click=(None, "Add tab to new group"))  # the menu's order changes between versions
time.sleep(1.5)
keys(text="hi")
keys("Return")
shot("29-tab-group", settle=1.5)
p.go("chrome://newtab")
p.js("chrome.send('setPalette', ['onyx']); chrome.send('clearWallpaper')")
time.sleep(1)
