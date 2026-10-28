import os, re, shutil, subprocess, sys
src = "/root/chromium/src"
work = "/root/pw"
shutil.rmtree(work, ignore_errors=True)
files = {}

def edit(path, fn):
    a = os.path.join(work, "a", path); b = os.path.join(work, "b", path)
    os.makedirs(os.path.dirname(a), exist_ok=True); os.makedirs(os.path.dirname(b), exist_ok=True)
    text = open(os.path.join(src, path)).read()
    open(a, "w").write(text)
    new = fn(text)
    assert new != text, f"no change in {path}"
    open(b, "w").write(new)
    files.setdefault(PATCH, []).append(path)

def once(text, old, new):
    assert text.count(old) == 1, f"anchor count {text.count(old)}: {old[:60]!r}"
    return text.replace(old, new)

def add_dep(text, anchor_target, dep):
    # Insert `dep` as the first entry of the first `deps = [` after the target header.
    i = text.index(anchor_target)
    j = text.index("  deps = [\n", i)
    k = j + len("  deps = [\n")
    return text[:k] + f'    "{dep}",\n' + text[k:]

PATCH = "mast-hooks.patch"
# 1. Navigation throttle
edit("chrome/browser/chrome_content_browser_client_navigation_throttles.cc", lambda t: once(once(t,
    '#include "chrome/browser/supervised_user/classify_url_navigation_throttle.h"\n',
    '#include "chrome/browser/supervised_user/classify_url_navigation_throttle.h"\n#include "mast/browser/mast_navigation_throttle.h"\n'),
    '  supervised_user::ClassifyUrlNavigationThrottle::MaybeCreateAndAdd(registry);\n',
    '  supervised_user::ClassifyUrlNavigationThrottle::MaybeCreateAndAdd(registry);\n  mast::MastNavigationThrottle::MaybeCreateAndAdd(registry);\n'))
edit("chrome/browser/BUILD.gn", lambda t: add_dep(t, 'source_set("core") {', "//mast/browser"))
# 2. Local State prefs + incognito lock
edit("chrome/browser/prefs/browser_prefs.cc", lambda t: once(once(t,
    '#include "chrome/browser/prefs/incognito_mode_prefs.h"\n',
    '#include "chrome/browser/prefs/incognito_mode_prefs.h"\n#include "mast/browser/mast_session.h"\n'),
    '  BrowserProcessImpl::RegisterPrefs(registry);\n',
    '  BrowserProcessImpl::RegisterPrefs(registry);\n  mast::MastSession::RegisterLocalStatePrefs(registry);\n'))
edit("chrome/browser/prefs/incognito_mode_prefs.cc", lambda t: once(once(t,
    '#include "chrome/browser/prefs/incognito_mode_prefs.h"\n',
    '#include "chrome/browser/prefs/incognito_mode_prefs.h"\n\n#include "mast/browser/mast_session.h"\n'),
    '    GetAvailabilityMode mode) {\n  DCHECK(profile);\n',
    '    GetAvailabilityMode mode) {\n  DCHECK(profile);\n  // Mast: no private windows during a deep-work session.\n  if (mast::IsSessionActive()) {\n    return policy::IncognitoModeAvailability::kDisabled;\n  }\n'))
edit("chrome/browser/prefs/BUILD.gn", lambda t: add_dep(t, 'source_set("impl") {', "//mast/browser"))
# 3. chrome://mast
edit("chrome/browser/ui/webui/chrome_web_ui_configs.cc", lambda t: once(once(t,
    '#include "content/public/browser/webui_config_map.h"\n',
    '#include "content/public/browser/webui_config_map.h"\n#include "mast/browser/mast_ui.h"\n'),
    '  auto& map = content::WebUIConfigMap::GetInstance();\n',
    '  auto& map = content::WebUIConfigMap::GetInstance();\n  map.AddWebUIConfig(std::make_unique<mast::MastUIConfig>());\n'))
edit("chrome/browser/ui/webui/BUILD.gn", lambda t: add_dep(t, 'source_set("configs") {', "//mast/browser"))

out = subprocess.run(["diff", "-ruN", "a", "b"], cwd=work, capture_output=True, text=True).stdout
open(os.path.join(work, PATCH), "w").write(out)
print(out)
