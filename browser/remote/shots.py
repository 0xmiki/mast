"""Drive Mast through fixed scenes and screenshot the whole (virtual) screen.

CDP navigates pages; xdotool sends real key presses for native UI (omnibox, app menu).
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
shot("01-newtab")
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
p.go("chrome://mast")
p.js("chrome.send('start', [25])")
shot("08-session", 2.5)
p.go("https://www.youtube.com/", 4)
shot("09-blocked")
