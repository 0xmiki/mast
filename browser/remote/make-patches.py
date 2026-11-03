"""Regenerate Mast's upstream patches against the base tree (git HEAD in src: Chromium plus
ungoogled-chromium's core patches and domain substitution; see remote/ungoogle.sh).

Run on the builder: python3 ~/mast/remote/make-patches.py  -> /root/pw/*.patch
Each edit asserts its anchor is found exactly once, so a Chromium upgrade that
moves code fails loudly here instead of silently producing a broken patch.
"""
import os, re, shutil, subprocess

SRC = "/root/chromium/src"
WORK = "/root/pw"
shutil.rmtree(WORK, ignore_errors=True)


def pristine(path):
    return subprocess.run(["git", "-C", SRC, "show", f"HEAD:{path}"],
                          capture_output=True, text=True, check=True).stdout


def once(text, old, new):
    n = text.count(old)
    assert n == 1, f"anchor found {n}x: {old[:70]!r}"
    return text.replace(old, new)


def add_dep(text, target_header, dep):
    i = text.index(target_header)
    j = text.index("  deps = [\n", i) + len("  deps = [\n")
    return text[:j] + f'    "{dep}",\n' + text[j:]


PATCHES = {}


def edit(patch, path, fn):
    a, b = (os.path.join(WORK, patch + ".d", side, path) for side in ("a", "b"))
    for p in (a, b):
        os.makedirs(os.path.dirname(p), exist_ok=True)
    text = pristine(path)
    new = fn(text)
    assert new != text, f"no change in {path}"
    open(a, "w").write(text)
    open(b, "w").write(new)
    PATCHES.setdefault(patch, []).append(path)


# ---------------------------------------------------------------- hooks
H = "mast-hooks.patch"

edit(H, "chrome/browser/chrome_content_browser_client_navigation_throttles.cc", lambda t: once(once(t,
    '#include "chrome/browser/supervised_user/classify_url_navigation_throttle.h"\n',
    '#include "chrome/browser/supervised_user/classify_url_navigation_throttle.h"\n#include "mast/browser/mast_navigation_throttle.h"\n'),
    '  supervised_user::ClassifyUrlNavigationThrottle::MaybeCreateAndAdd(registry);\n',
    '  supervised_user::ClassifyUrlNavigationThrottle::MaybeCreateAndAdd(registry);\n  mast::MastNavigationThrottle::MaybeCreateAndAdd(registry);\n'))
edit(H, "chrome/browser/BUILD.gn", lambda t: add_dep(t, 'source_set("core") {', "//mast/browser"))

edit(H, "chrome/browser/prefs/browser_prefs.cc", lambda t: once(once(t,
    '#include "chrome/browser/prefs/incognito_mode_prefs.h"\n',
    '#include "chrome/browser/prefs/incognito_mode_prefs.h"\n#include "mast/browser/mast_blocks.h"\n'),
    '  BrowserProcessImpl::RegisterPrefs(registry);\n',
    '  BrowserProcessImpl::RegisterPrefs(registry);\n  mast::MastBlocks::RegisterLocalStatePrefs(registry);\n'))
edit(H, "chrome/browser/prefs/incognito_mode_prefs.cc", lambda t: once(once(t,
    '#include "chrome/browser/prefs/incognito_mode_prefs.h"\n',
    '#include "chrome/browser/prefs/incognito_mode_prefs.h"\n\n#include "mast/browser/mast_blocks.h"\n'),
    '    GetAvailabilityMode mode) {\n  DCHECK(profile);\n',
    '    GetAvailabilityMode mode) {\n  DCHECK(profile);\n  // Mast: no private windows while any site is blocked.\n  if (mast::AnyBlockActive()) {\n    return policy::IncognitoModeAvailability::kDisabled;\n  }\n'))
edit(H, "chrome/browser/prefs/BUILD.gn", lambda t: add_dep(t, 'source_set("impl") {', "//mast/browser"))

# chrome://mast, and Mast's page as the new tab page (both NTP hosts, so the
# omnibox still treats it as the NTP).
edit(H, "chrome/browser/ui/webui/chrome_web_ui_configs.cc", lambda t: once(once(once(once(t,
    '#include "content/public/browser/webui_config_map.h"\n',
    '#include "content/public/browser/webui_config_map.h"\n#include "mast/browser/mast_ui.h"\n'),
    '  auto& map = content::WebUIConfigMap::GetInstance();\n',
    '  auto& map = content::WebUIConfigMap::GetInstance();\n  map.AddWebUIConfig(std::make_unique<mast::MastUIConfig>(mast::kMastHost));\n'),
    '  map.AddWebUIConfig(std::make_unique<NewTabPageUIConfig>());\n',
    '  map.AddWebUIConfig(\n      std::make_unique<mast::MastUIConfig>(chrome::kChromeUINewTabPageHost));\n'),
    '  map.AddWebUIConfig(std::make_unique<NewTabPageThirdPartyUIConfig>());\n',
    '  map.AddWebUIConfig(std::make_unique<mast::MastUIConfig>(\n      chrome::kChromeUINewTabPageThirdPartyHost));\n'))
edit(H, "chrome/browser/ui/webui/BUILD.gn", lambda t: add_dep(t, 'source_set("configs") {', "//mast/browser"))

# Toolbar: the Block button for the current site, just before the avatar.
edit(H, "chrome/browser/ui/views/toolbar/toolbar_view.cc", lambda t: once(once(t,
    '#include "chrome/browser/ui/views/toolbar/toolbar_view.h"\n',
    '#include "chrome/browser/ui/views/toolbar/toolbar_view.h"\n\n#include "mast/browser/ui/mast_toolbar_button.h"\n'),
    '  if (!features::IsWebUIAvatarButtonEnabled()) {\n    avatar_ = AddChildView',
    '  AddChildView(std::make_unique<mast::MastToolbarButton>(browser_));\n\n  if (!features::IsWebUIAvatarButtonEnabled()) {\n    avatar_ = AddChildView').replace(
    '        AvatarToolbarButtonInterface::CanShowForProfile(browser_->GetProfile());\n',
    '        false;  // Mast: no sign-in, so no profile button.\n', 1))
edit(H, "chrome/browser/ui/views/toolbar/BUILD.gn", lambda t: add_dep(once(t,
    '    "toolbar_view.cc",\n',
    '    "toolbar_view.cc",\n    "//mast/browser/ui/mast_toolbar_button.cc",\n    "//mast/browser/ui/mast_toolbar_button.h",\n'),
    'source_set("impl") {', "//mast/browser"))

# The toolbar Block bubbles subclass BubbleDialogDelegateView, whose constructor is
# limited to a friend list (like the home button's undo bubble).
edit(H, "ui/views/bubble/bubble_dialog_delegate_view.h", lambda t: once(once(t,
    'class HomePageUndoBubble;\n',
    'class HomePageUndoBubble;\nnamespace mast {\nclass MastBlockBubble;\nclass MastUndoBubble;\n}  // namespace mast\n'),
    '  friend class ::HomePageUndoBubble;\n',
    '  friend class ::HomePageUndoBubble;\n  friend class ::mast::MastBlockBubble;\n  friend class ::mast::MastUndoBubble;\n'))

# Theme: dark and not the GTK theme on Linux. The colours themselves come from
# Mast's palette (mast-theme.patch), so no seeded user colour.
def theme(t):
    t = once(t, 'registry->RegisterIntegerPref(\n      prefs::kBrowserColorScheme,\n      std::to_underlying(ThemeService::BrowserColorScheme::kSystem));',
             'registry->RegisterIntegerPref(\n      prefs::kBrowserColorScheme,\n      std::to_underlying(ThemeService::BrowserColorScheme::kDark));')
    t = once(t, 'static_cast<int>(ui::GetDefaultSystemTheme()));',
             'static_cast<int>(ui::SystemTheme::kDefault));')
    return t
edit(H, "chrome/browser/themes/theme_service.cc", theme)

# ---------------------------------------------------------------- theme
T = "mast-theme.patch"

# Chromium's own cool dark neutrals (match the demo) and surface tokens.
edit(T, "ui/base/ui_base_features.cc", lambda t: once(once(t,
    'BASE_FEATURE(kChromeDarkNeutrals26, base::FEATURE_DISABLED_BY_DEFAULT);',
    'BASE_FEATURE(kChromeDarkNeutrals26, base::FEATURE_ENABLED_BY_DEFAULT);'),
    'BASE_FEATURE(kDarkModeSurfaceTokens, base::FEATURE_DISABLED_BY_DEFAULT);',
    'BASE_FEATURE(kDarkModeSurfaceTokens, base::FEATURE_ENABLED_BY_DEFAULT);'))

# Mast accent palette, right after the reference palette.
edit(T, "ui/color/color_mixers.cc", lambda t: once(once(t,
    '#include "ui/color/color_mixers.h"\n',
    '#include "ui/color/color_mixers.h"\n\n#include "mast/ui/color/mast_ref_color_mixer.h"\n'),
    '  AddRefColorMixer(provider, key);\n',
    '  AddRefColorMixer(provider, key);\n  mast::AddMastRefColorMixer(provider, key);\n'))
edit(T, "ui/color/BUILD.gn", lambda t: once(t,
    '    "color_mixers.cc",\n',
    '    "color_mixers.cc",\n    "//mast/ui/color/mast_palette.h",\n    "//mast/ui/color/mast_ref_color_mixer.cc",\n    "//mast/ui/color/mast_ref_color_mixer.h",\n'))

# Mast surfaces, after all of Chrome's mixers.
edit(T, "chrome/browser/ui/color/chrome_color_mixers.cc", lambda t: once(once(t,
    '#include "chrome/browser/ui/color/chrome_color_mixers.h"\n',
    '#include "chrome/browser/ui/color/chrome_color_mixers.h"\n\n#include "mast/browser/ui/color/mast_chrome_color_mixer.h"\n'),
    '  AddMaterialTabStripColorMixer(provider, key);\n',
    '  AddMaterialTabStripColorMixer(provider, key);\n  mast::AddMastChromeColorMixer(provider, key);\n'))
edit(T, "chrome/browser/ui/color/BUILD.gn", lambda t: once(t,
    '    "chrome_color_mixers.cc",\n',
    '    "chrome_color_mixers.cc",\n    "//mast/browser/ui/color/mast_chrome_color_mixer.cc",\n    "//mast/browser/ui/color/mast_chrome_color_mixer.h",\n'))

# Calm UI: no ink ripples (the demo has none), no profile button (no sign-in),
# Inter for internal pages instead of Roboto.
edit(T, "ui/views/style/platform_style.h", lambda t: once(t,
    'static constexpr bool kUseRipples = !BUILDFLAG(IS_MAC);',
    'static constexpr bool kUseRipples = false;  // Mast: no ink ripples.'))
edit(T, "ui/base/webui/web_ui_util.cc", lambda t: once(t,
    'return "Roboto, " + GetFontFamily();', 'return "Inter, " + GetFontFamily();'))

# ---------------------------------------------------------------- branding
B = "mast-branding.patch"

# Product names in UI strings are renamed at build time by substitute-names.py
# (every .grd/.grdp), not by patch.

# The shared WebUI "chrome-product" glyph (Settings header, About) -> Mast sails.
SAILS = "M10 7.25V21.25H2.6Q3.4 12.75 10 7.25ZM13 2.75V21.25H21.4Q20.7 9.75 13 2.75Z"
def sails_glyph(t):
    t, n1 = re.subn(r'(<g id="chrome-product">\s*<path d=")[^"]*(")', r"\g<1>" + SAILS + r"\g<2>", t)
    t, n2 = re.subn(r'<g id="chrome-product" viewBox="0 -960 960 960">\s*<path d="[^"]*">\s*</path>',
                    '<g id="chrome-product" viewBox="0 0 24 24">\n        <path d="' + SAILS + '"></path>', t)
    assert (n1, n2) == (1, 1), (n1, n2)
    return t
edit(B, "ui/webui/resources/cr_elements/icons.html.ts", sails_glyph)

# Linux identity: desktop file (and so WM_CLASS) is "mast".
edit(B, "chrome/common/channel_info_posix.cc", lambda t: once(t,
    'return "chromium-browser.desktop";', 'return "mast.desktop";'))
edit(B, "chrome/common/chrome_paths_linux.cc", lambda t: once(t,
    'std::string data_dir_basename = "chromium";', 'std::string data_dir_basename = "mast";'))
edit(B, "chrome/browser/ui/startup/infobar_utils.cc", lambda t: once(t,
    '  if (!google_apis::HasAPIKeyConfigured()) {\n',
    '  // Mast doesn\'t use Google API keys, so never nag about missing ones.\n  if (false) {\n'))

# ---------------------------------------------------------------- ungoogled adjustments
U = "mast-ungoogled.patch"

# Google stays the default search engine. Domain substitution rewrote kGoogleHomepageURL
# (to a dead *.qjz9zk host), and the Google engine's search and suggest URLs are built from
# this one value. Only searches the user types go to Google; background services stay off.
edit(U, "components/search_engines/search_terms_data.cc", lambda t: once(t,
    '  return google_util::kGoogleHomepageURL;\n',
    '  // Mast: searches the user types still go to Google (see mast-ungoogled.patch).\n  return "https://www.google.com/";\n'))

# Page preloading off by default. With Google as the search engine, Chromium otherwise
# prefetches Google's results page (and its trackers) while you type in the omnibox.
# People can still turn it on in Settings > Performance.
edit(U, "chrome/browser/preloading/preloading_prefs.cc", lambda t: once(t,
    '      static_cast<int>(NetworkPredictionOptions::kDefault),\n',
    '      static_cast<int>(NetworkPredictionOptions::kDisabled),  // Mast\n'))

for patch in PATCHES:
    out = subprocess.run(["diff", "-ruN", "a", "b"], cwd=os.path.join(WORK, patch + ".d"),
                         capture_output=True, text=True).stdout
    # Drop timestamps from ---/+++ headers so a patch only changes when its content does.
    out = re.sub(r"^(---|\+\+\+) (\S+)\t.*$", r"\1 \2", out, flags=re.M)
    open(os.path.join(WORK, patch), "w").write(out)
    print(f"{patch}: {len(PATCHES[patch])} files, +{sum(1 for l in out.splitlines() if l.startswith('+') and not l.startswith('+++'))} lines")
