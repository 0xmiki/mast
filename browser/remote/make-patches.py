"""Regenerate Mast's upstream patches against the base tree (git HEAD in src: Chromium plus
ungoogled-chromium's core patches and domain substitution; see remote/ungoogle.sh).

Run on the builder: python3 ~/mast/remote/make-patches.py  -> /root/pw/*.patch
Each edit asserts its anchor is found exactly once, so a Chromium upgrade that
moves code fails loudly here instead of silently producing a broken patch.
"""
import os, re, shutil, subprocess

SRC = "/root/chromium/src"
WORK = os.environ.get("MAST_PATCH_WORK", "/root/pw")
# Local mode (./mast check-patches): read base files rebuilt from Chromium's source site plus
# ungoogled's patches by tools/base-file.mjs, so anchors are checked without the builder.
LOCAL = os.environ.get("MAST_LOCAL_BASE")
shutil.rmtree(WORK, ignore_errors=True)


def pristine(path):
    if LOCAL:
        local = subprocess.run(["node", os.path.join(LOCAL, "tools/base-file.mjs"), path],
                               capture_output=True, text=True, check=True).stdout.strip()
        return open(local).read()
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
    base = pristine(path)
    # A file edited twice in one patch: the second edit builds on the first
    # (starting from the pristine file again silently dropped the first).
    text = open(b).read() if path in PATCHES.get(patch, []) else base
    new = fn(text)
    assert new != text, f"no change in {path}"
    open(a, "w").write(base)
    open(b, "w").write(new)
    if path not in PATCHES.setdefault(patch, []):
        PATCHES[patch].append(path)


# ---------------------------------------------------------------- hooks
H = "mast-hooks.patch"

edit(H, "chrome/browser/chrome_content_browser_client_navigation_throttles.cc", lambda t: once(once(t,
    '#include "chrome/browser/supervised_user/classify_url_navigation_throttle.h"\n',
    '#include "chrome/browser/supervised_user/classify_url_navigation_throttle.h"\n#include "mast/browser/mast_navigation_throttle.h"\n'),
    '  supervised_user::ClassifyUrlNavigationThrottle::MaybeCreateAndAdd(registry);\n',
    '  supervised_user::ClassifyUrlNavigationThrottle::MaybeCreateAndAdd(registry);\n  mast::MastNavigationThrottle::MaybeCreateAndAdd(registry);\n'))
edit(H, "chrome/browser/BUILD.gn", lambda t: add_dep(t, 'source_set("core") {', "//mast/browser"))

edit(H, "chrome/browser/prefs/browser_prefs.cc", lambda t: once(once(once(t,
    '#include "chrome/browser/prefs/incognito_mode_prefs.h"\n',
    '#include "chrome/browser/prefs/incognito_mode_prefs.h"\n#include "mast/browser/mast_blocks.h"\n#include "mast/browser/mast_prefs.h"\n'),
    '  BrowserProcessImpl::RegisterPrefs(registry);\n',
    '  BrowserProcessImpl::RegisterPrefs(registry);\n  mast::MastBlocks::RegisterLocalStatePrefs(registry);\n'),
    # Last in RegisterProfilePrefs(), so Mast can change upstream defaults.
    '  registry->RegisterBooleanPref(prefs::kStaticStorageQuotaEnabled, false);\n}\n',
    '  registry->RegisterBooleanPref(prefs::kStaticStorageQuotaEnabled, false);\n  mast::RegisterProfilePrefs(registry);\n}\n'))
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
    '#include "content/public/browser/webui_config_map.h"\n#include "mast/browser/webui/mast_ui.h"\n'),
    '  auto& map = content::WebUIConfigMap::GetInstance();\n',
    '  auto& map = content::WebUIConfigMap::GetInstance();\n  map.AddWebUIConfig(std::make_unique<mast::MastUIConfig>(mast::kMastHost));\n'),
    '  map.AddWebUIConfig(std::make_unique<NewTabPageUIConfig>());\n',
    '  map.AddWebUIConfig(\n      std::make_unique<mast::MastUIConfig>(chrome::kChromeUINewTabPageHost));\n'),
    '  map.AddWebUIConfig(std::make_unique<NewTabPageThirdPartyUIConfig>());\n',
    '  map.AddWebUIConfig(std::make_unique<mast::MastUIConfig>(\n      chrome::kChromeUINewTabPageThirdPartyHost));\n'))
edit(H, "chrome/browser/ui/webui/BUILD.gn", lambda t: add_dep(t, 'source_set("configs") {', "//mast/browser/webui"))

# The star (and Ctrl+D) saves to the Desktop folder behind the new tab page. Mast's
# provider takes the slot of the shopping one (commerce is off in Mast); the shopping
# provider stays as the fallback so upstream code is unchanged and still compiles.
edit(H, "chrome/browser/bookmarks/chrome_bookmark_client.cc", lambda t: once(once(t,
    '#include "chrome/browser/bookmarks/chrome_bookmark_client.h"\n',
    '#include "chrome/browser/bookmarks/chrome_bookmark_client.h"\n\n#include "mast/browser/desktop/mast_desktop.h"\n'),
    '  shopping_save_location_provider_ =\n      std::make_unique<ShoppingCollectionProvider>(model, profile_);\n',
    '  // Mast: new bookmarks go to the Desktop folder (the new tab page).\n'
    '  shopping_save_location_provider_ =\n      mast::CreateDesktopSaveLocationProvider(model);\n'
    '  if (!shopping_save_location_provider_) {\n'
    '    shopping_save_location_provider_ =\n        std::make_unique<ShoppingCollectionProvider>(model, profile_);\n  }\n'))
edit(H, "chrome/browser/bookmarks/BUILD.gn", lambda t: add_dep(t, 'source_set("impl") {', "//mast/browser/desktop"))

# Toolbar: no profile button (Mast has no sign-in).
edit(H, "chrome/browser/ui/views/toolbar/toolbar_view.cc", lambda t: once(t,
    '        AvatarToolbarButtonInterface::CanShowForProfile(browser_->GetProfile());\n',
    '        false;  // Mast: no sign-in, so no profile button.\n'))

# Page info (the icon at the start of the address bar): "Block this site", after the
# extensions row. It opens Mast's block bubble from the same icon.
edit(H, "chrome/browser/ui/views/page_info/page_info_main_view.cc", lambda t: once(once(t,
    '#include "chrome/browser/ui/views/page_info/page_info_main_view.h"\n',
    '#include "chrome/browser/ui/views/page_info/page_info_main_view.h"\n\n#include "mast/browser/ui/mast_block_bubble.h"\n'),
    '        views::style::STYLE_BODY_4, kColorPageInfoSubtitleForeground);\n  }\n\n  int link_text_id = 0;\n',
    '        views::style::STYLE_BODY_4, kColorPageInfoSubtitleForeground);\n  }\n\n'
    '  // Mast: "Block this site".\n'
    '  site_settings_view_->AddChildView(mast::CreateBlockSiteRow(\n'
    '      ui_delegate_->GetWebContents(),\n'
    '      base::BindRepeating(\n'
    '          [](PageInfoNavigationHandler* handler) {\n'
    '            if (handler) {\n              handler->CloseBubble();\n            }\n'
    '          },\n          base::Unretained(navigation_handler_))));\n\n'
    '  int link_text_id = 0;\n'))
edit(H, "chrome/browser/ui/views/page_info/BUILD.gn", lambda t: add_dep(once(t,
    '    "page_info_main_view.cc",\n',
    '    "page_info_main_view.cc",\n    "//mast/browser/ui/mast_block_bubble.cc",\n    "//mast/browser/ui/mast_block_bubble.h",\n'),
    'source_set("impl") {', "//mast/browser"))

# Mast's block bubbles subclass BubbleDialogDelegateView, whose constructor is
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

# Settings without Google: no "You and Google" page (account, Google services, import),
# no AI page, no Privacy Guide. Import moves to Mast's own onboarding (IMPORT-PLAN.md).
edit(H, "chrome/browser/ui/webui/settings/settings_ui.cc", lambda t: once(t,
    '  TryShowHatsSurveyWithTimeout();\n}\n',
    '  // Mast: no AI or Google pages in Settings.\n'
    '  html_source->AddBoolean("showAiPage", false);\n'
    '  html_source->AddBoolean("showGlicSettings", false);\n'
    '  html_source->AddBoolean("showGeicSettings", false);\n'
    '  html_source->AddBoolean("showPrivacyGuide", false);\n\n'
    '  TryShowHatsSurveyWithTimeout();\n}\n'))
edit(H, "chrome/browser/resources/settings/page_visibility.ts", lambda t: once(t,
    "  if (!loadTimeData.getBoolean('isGuest')) {\n    return undefined;\n  }\n",
    "  if (!loadTimeData.getBoolean('isGuest')) {\n"
    "    // Mast: no Google account page and no AI page. Every key is listed: the\n"
    "    // menu hides pages whose entry is missing.\n"
    "    return {\n"
    "      a11y: true, ai: false, autofill: true, defaultBrowser: true, downloads: true,\n"
    "      extensions: true, languages: true, onStartup: true, people: false,\n"
    "      performance: true, reset: true, safetyHub: true, system: true, yourSavedInfo: true,\n"
    "      appearance: {bookmarksBar: true, homeButton: true, hoverCard: true, pageZoom: true,\n"
    "                   setTheme: true, sidePanel: true},\n"
    "      privacy: {networkPrediction: true, searchPrediction: true},\n"
    "    };\n  }\n"))

# mast:// for Mast's own pages, like Brave's brave://. Internally they stay chrome://
# pages: the scheme is registered (so the omnibox treats mast://settings as a URL),
# typing mast:// is rewritten to chrome:// first, and the omnibox shows chrome:// as mast://.
edit(H, "chrome/common/chrome_content_client.cc", lambda t: once(t,
    '  for (auto* standard_scheme : kChromeStandardURLSchemes)\n    schemes->standard_schemes.push_back(standard_scheme);\n',
    '  for (auto* standard_scheme : kChromeStandardURLSchemes)\n    schemes->standard_schemes.push_back(standard_scheme);\n'
    '  schemes->standard_schemes.push_back("mast");  // Mast: mast:// opens chrome:// pages.\n'))
edit(H, "chrome/browser/chrome_content_browser_client.cc", lambda t: once(once(t,
    '#include "chrome/browser/chrome_content_browser_client.h"\n',
    '#include "chrome/browser/chrome_content_browser_client.h"\n\n#include "mast/browser/mast_scheme.h"\n'),
    '    BrowserURLHandler* handler) {\n  // The group policy NTP URL handler must be registered before',
    '    BrowserURLHandler* handler) {\n  // Mast: mast://x -> chrome://x, before every other handler.\n'
    '  handler->AddHandlerPair(&mast::HandleMastScheme,\n                          BrowserURLHandler::null_handler());\n\n'
    '  // The group policy NTP URL handler must be registered before'))
edit(H, "components/omnibox/browser/location_bar_model_impl.cc", lambda t: once(t,
    '  return gfx::TruncateString(formatted_text, max_url_display_chars_,\n                             gfx::CHARACTER_BREAK);\n',
    '  // Mast: its own pages read mast://, not chrome://.\n'
    '  std::u16string shown = formatted_text;\n'
    '  if (url.SchemeIs("chrome") && shown.starts_with(u"chrome://")) {\n'
    '    shown.replace(0, 6, u"mast");\n  }\n'
    '  return gfx::TruncateString(shown, max_url_display_chars_,\n                             gfx::CHARACTER_BREAK);\n'))
# Typing mast://version in the address bar: the omnibox only treats schemes the browser
# handles itself as URLs, and mast:// is only rewritten at navigation, so it searched.
edit(H, "chrome/browser/autocomplete/chrome_autocomplete_scheme_classifier.cc", lambda t: once(t,
    '       base::EqualsCaseInsensitiveASCII(scheme, content::kViewSourceScheme) ||\n',
    '       base::EqualsCaseInsensitiveASCII(scheme, content::kViewSourceScheme) ||\n'
    '       // Mast: mast:// is chrome:// (mast/browser/mast_scheme.h).\n'
    '       base::EqualsCaseInsensitiveASCII(scheme, "mast") ||\n'))

# New tabs show the Mast new-tab icon from the start. Before the page commits the favicon
# driver has no icon yet, and the tab showed Chromium's default globe for a moment.
edit(H, "chrome/browser/ui/tab_ui_helper.cc", lambda t: once(once(t,
    '#include "chrome/grit/generated_resources.h"\n',
    '#include "chrome/grit/generated_resources.h"\n#include "chrome/grit/theme_resources.h"\n'),
    '  if (!favicon_driver || !favicon_driver->FaviconIsValid()) {\n',
    '  if (!favicon_driver || !favicon_driver->FaviconIsValid()) {\n'
    '    // Mast: a new tab shows the new-tab icon while it loads, not the globe.\n'
    '    if (IsNTP(web_contents()->GetVisibleURL())) {\n'
    '      return ui::ImageModel::FromResourceId(IDR_NTP_FAVICON);\n    }\n'))
edit(H, "chrome/browser/ui/BUILD.gn", lambda t: add_dep(t, '  source_set("tab_ui_helper") {', "//chrome/app/theme:theme_resources"))

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

# Settings (and other WebUI pages) paint their page and cards from two hard-coded dark
# greys in shared CSS, so the palette never reached them. They read the browser's colours
# instead (chrome://theme/colors.css, which Mast's mixer sets per palette), keeping the old
# values for pages that don't load colors.css.
edit(T, "ui/webui/resources/css/md_colors.css", lambda t: once(t,
    '    --md-background-color: rgb(32, 33, 36);  /* --google-grey-900 */\n',
    '    /* Mast: the palette\'s page colour. */\n'
    '    --md-background-color: var(--color-webui-page-background, rgb(32, 33, 36));\n'))
edit(T, "ui/webui/resources/cr_elements/cr_shared_vars.css", lambda t: once(t,
    '    --cr-card-background-color: var(--google-grey-900-white-4-percent);\n',
    '    /* Mast: the palette\'s raised surface. */\n'
    '    --cr-card-background-color: var(--color-webui-card-background,\n'
    '        var(--google-grey-900-white-4-percent));\n'))

# Settings only loads the browser's colours (chrome://theme/colors.css) behind Chromium's
# 2026 redesign flags; without them it paints Google's greys and blues, whatever the
# palette. It always loads them in Mast, and follows palette changes live.
edit(T, "chrome/browser/resources/settings/settings_ui/settings_ui.ts", lambda t: once(t,
    "    const enableThemedColors =\n"
    "        loadTimeData.getString('webuiRefresh2026') !== '' ||\n"
    "        loadTimeData.getString('settingsRefresh2026') !== '';\n",
    "    // Mast: always themed, so Settings follows the palette.\n"
    "    const enableThemedColors = true;\n"))
# The selected Settings menu item was Google blue; it's the palette's raised surface.
edit(T, "ui/webui/resources/cr_elements/cr_nav_menu_item_style_lit.css", lambda t: once(t,
    '    --cr-nav-menu-item-color: white;\n'
    '    --cr-nav-menu-item-color-selected: var(--google-grey-900);\n\n'
    '    --cr-nav-menu-item-background-color-hover: var(--google-grey-800);\n'
    '    --cr-nav-menu-item-background-color-selected: var(--google-blue-300);\n\n'
    '    --cr-nav-menu-item-icon-color: var(--google-grey-500);\n'
    '    --cr-nav-menu-item-icon-color-hover: white;\n'
    '    --cr-nav-menu-item-icon-color-selected: black;\n',
    '    /* Mast: the palette (colors.css), Chromium\'s values as fallbacks. */\n'
    '    --cr-nav-menu-item-color: var(--color-primary-foreground, white);\n'
    '    --cr-nav-menu-item-color-selected: var(--color-primary-foreground,\n'
    '        var(--google-grey-900));\n\n'
    '    --cr-nav-menu-item-background-color-hover: var(\n'
    '        --color-webui-card-background, var(--google-grey-800));\n'
    '    --cr-nav-menu-item-background-color-selected: var(\n'
    '        --color-menu-item-background-selected, var(--google-blue-300));\n\n'
    '    --cr-nav-menu-item-icon-color: var(--color-secondary-foreground,\n'
    '        var(--google-grey-500));\n'
    '    --cr-nav-menu-item-icon-color-hover: var(--color-primary-foreground, white);\n'
    '    --cr-nav-menu-item-icon-color-selected: var(--color-primary-foreground,\n'
    '        black);\n'))

# Mast surfaces, after all of Chrome's mixers.
edit(T, "chrome/browser/ui/color/chrome_color_mixers.cc", lambda t: once(once(t,
    '#include "chrome/browser/ui/color/chrome_color_mixers.h"\n',
    '#include "chrome/browser/ui/color/chrome_color_mixers.h"\n\n#include "mast/browser/ui/color/mast_chrome_color_mixer.h"\n'),
    '  AddMaterialTabStripColorMixer(provider, key);\n',
    '  AddMaterialTabStripColorMixer(provider, key);\n  mast::AddMastChromeColorMixer(provider, key);\n'))
edit(T, "chrome/browser/ui/color/BUILD.gn", lambda t: add_dep(once(t,
    '    "chrome_color_mixers.cc",\n',
    '    "chrome_color_mixers.cc",\n    "//mast/browser/ui/color/mast_chrome_color_mixer.cc",\n    "//mast/browser/ui/color/mast_chrome_color_mixer.h",\n'),
    'source_set("mixers") {', "//mast/common"))

# Calm UI: no ink ripples (the demo has none), no profile button (no sign-in),
# Inter for internal pages instead of Roboto.
edit(T, "ui/views/style/platform_style.h", lambda t: once(t,
    'static constexpr bool kUseRipples = !BUILDFLAG(IS_MAC);',
    'static constexpr bool kUseRipples = false;  // Mast: no ink ripples.'))
edit(T, "ui/base/webui/web_ui_util.cc", lambda t: once(t,
    'return "Roboto, " + GetFontFamily();', 'return "Inter, " + GetFontFamily();'))

# Native UI shape (docs/NATIVE-UI.md). The omnibox focus ring gets its own colour id,
# which skips Chromium's 3:1 contrast lift, and a 1px halo: a hairline, not a glow.
edit(T, "chrome/browser/ui/views/location_bar/location_bar_view.cc", lambda t: once(t,
    '    views::FocusRing::Get(this)->SetOutsetFocusRingDisabled(true);\n',
    '    views::FocusRing::Get(this)->SetOutsetFocusRingDisabled(true);\n'
    '    // Mast: a hairline focus ring (kColorLocationBarBorder in Mast\'s mixer).\n'
    '    views::FocusRing::Get(this)->SetColorId(kColorLocationBarBorder);\n'
    '    views::FocusRing::Get(this)->SetHaloThickness(1.f);\n'))

# One radius table for dialogs, menus, comboboxes, textfields and buttons: tighter
# corners (Mast's bubbles are 10px, its buttons 6px; Chromium's are 12-16px and pills).
def radii(t):
    t = once(t, '    case ShapeSysTokens::kSmall:\n      return 8;\n', '    case ShapeSysTokens::kSmall:\n      return 6;  // Mast\n')
    t = once(t, '    case ShapeSysTokens::kMediumSmall:\n      return 12;\n', '    case ShapeSysTokens::kMediumSmall:\n      return 10;  // Mast\n')
    t = once(t, '    case ShapeSysTokens::kMedium:\n      return 16;\n', '    case ShapeSysTokens::kMedium:\n      return 10;  // Mast\n')
    t = once(t, '          {ShapeContextTokens::kButtonRadius, ShapeSysTokens::kFull},\n',
             '          {ShapeContextTokens::kButtonRadius, ShapeSysTokens::kSmall},  // Mast\n')
    return t
edit(T, "ui/views/layout/layout_provider.cc", radii)

# Inter for the native UI too (fontconfig falls back to the system sans without it).
edit(T, "ui/gfx/platform_font_skia.cc", lambda t: once(t,
    '    params = linux_ui->GetDefaultFontRenderParams();\n',
    '    params = linux_ui->GetDefaultFontRenderParams();\n    family = "Inter";  // Mast\n'))

# Tabs: 8px top corners and a small foot (4px) instead of Chrome's swoop. The foot
# stays above the separator width so the tab-overlap maths stays positive.
edit(T, "chrome/browser/ui/tabs/tab_style.cc", lambda t: once(once(t,
    'int TabStyle::GetTopCornerRadius() const {\n  return 10;\n',
    'int TabStyle::GetTopCornerRadius() const {\n  return 8;  // Mast\n'),
    'int TabStyle::GetBottomCornerRadius() const {\n  return 12;\n',
    'int TabStyle::GetBottomCornerRadius() const {\n  return 4;  // Mast\n'))

# ---------------------------------------------------------------- one row
# Tabs, then back / address / forward, then the menu, in one row that is also the
# titlebar (mast/common/mast_features.h; --disable-features=MastOneRowWindow for two
# rows). The geometry is mast/browser/ui/mast_one_row_layout.h; these are the hooks.
R = "mast-onerow.patch"

# Tabs take the row up to where the toolbar starts; the toolbar's top container is
# exactly the slice at the end, so it never paints over the tabs.
edit(R, "chrome/browser/ui/views/frame/layout/browser_view_tabbed_layout_impl.cc", lambda t: once(once(once(t,
    '#include "chrome/browser/ui/views/frame/layout/browser_view_tabbed_layout_impl.h"\n',
    '#include "chrome/browser/ui/views/frame/layout/browser_view_tabbed_layout_impl.h"\n\n#include "mast/browser/ui/mast_one_row_layout.h"\n'),
    '          params, views().horizontal_tab_strip_region_view, leading_margin);\n'
    '      params.SetTop(tabstrip_bounds.bottom() -\n'
    '                    GetLayoutConstant(LayoutConstant::kTabstripToolbarOverlap));\n'
    '      needs_exclusion = false;\n    }\n    layout.AddChild(views().horizontal_tab_strip_region_view, tabstrip_bounds,\n'
    '                    layout_data_->tab_strip_type == TabStripType::kHorizontal);\n  }\n\n  // This will be',
    '          params, views().horizontal_tab_strip_region_view, leading_margin);\n'
    '      if (mast::IsOneRowWindow()) {\n'
    '        // Mast: the toolbar shares this row (see CalculateTopContainerLayout).\n'
    '        params.SetTop(mast::FitTabStripInRow(\n'
    '            tabstrip_bounds, params.visual_client_area.x(),\n'
    '            GetLayoutConstant(LayoutConstant::kTabstripToolbarOverlap),\n'
    '            views().horizontal_tab_strip_region_view, views().toolbar));\n'
    '      } else {\n'
    '      params.SetTop(tabstrip_bounds.bottom() -\n'
    '                    GetLayoutConstant(LayoutConstant::kTabstripToolbarOverlap));\n'
    '      }\n      needs_exclusion = false;\n    }\n    layout.AddChild(views().horizontal_tab_strip_region_view, tabstrip_bounds,\n'
    '                    layout_data_->tab_strip_type == TabStripType::kHorizontal);\n  }\n\n  // This will be'),
    '    bool needs_exclusion) const {\n  const int original_top = params.visual_client_area.y();\n',
    '    bool needs_exclusion) const {\n  const int original_top = params.visual_client_area.y();\n\n'
    '  // Mast: one row. The top container is the toolbar\'s slice at the end of\n'
    '  // the tab row, just above this area (no bookmarks bar or separator in it).\n'
    '  if (const gfx::Rect slice =\n'
    '          mast::IsOneRowWindow() &&\n'
    '                  layout_data_->tab_strip_type == TabStripType::kHorizontal &&\n'
    '                  IsParentedTo(views().toolbar, views().top_container) &&\n'
    '                  IsParentedTo(views().horizontal_tab_strip_region_view,\n'
    '                               views().browser_view)\n'
    '              ? mast::TakeOneRowToolbarSlice(params.visual_client_area)\n'
    '              : gfx::Rect();\n'
    '      !slice.IsEmpty()) {\n'
    '    layout.AddChild(views().toolbar, gfx::Rect(slice.size()),\n'
    '                    delegate().IsToolbarVisible());\n'
    '    if (IsParentedTo(views().bookmark_bar, views().top_container)) {\n'
    '      layout.AddChild(views().bookmark_bar, gfx::Rect(), false);\n    }\n'
    '    if (IsParentedTo(views().top_container_separator, views().top_container)) {\n'
    '      layout.AddChild(views().top_container_separator, gfx::Rect(), false);\n    }\n'
    '    return slice;\n  }\n'))
edit(R, "chrome/browser/ui/views/frame/BUILD.gn", lambda t: add_dep(t, 'source_set("browser_view_tabbed_layout_impl") {', "//mast/common"))

# Tabs are pills: every tab shape (fill, border, clip) uses the detached rounded rect
# Chromium already draws for hover and hit tests, instead of the swoop into the toolbar.
edit(R, "chrome/browser/ui/views/tabs/common/horizontal_tab_style_views.cc", lambda t: once(once(once(t,
    '#include "chrome/browser/ui/views/tabs/common/horizontal_tab_style_views.h"\n',
    '#include "chrome/browser/ui/views/tabs/common/horizontal_tab_style_views.h"\n\n#include "mast/common/mast_features.h"\n'),
    '  if (path_type == TabStyle::PathType::kHighlight ||\n      path_type == TabStyle::PathType::kHitTest) {\n    float top_left_corner_radius',
    '  if (path_type == TabStyle::PathType::kHighlight ||\n      path_type == TabStyle::PathType::kHitTest ||\n'
    '      mast::IsOneRowWindow()) {  // Mast: pill tabs.\n    float top_left_corner_radius'),
    '    float right = aligned_bounds.right() - extension_corner_radius;\n    const int bottom = top + tab_height;\n',
    '    float right = aligned_bounds.right() - extension_corner_radius;\n'
    '    // Mast: in one row the pill is centred, with an even gap above and below.\n'
    '    if (mast::IsOneRowWindow() && path_type != TabStyle::PathType::kHitTest) {\n'
    '      top = aligned_bounds.y() + mast::kOneRowTabPillInset * scale;\n'
    '      tab_height =\n          aligned_bounds.height() - 2 * mast::kOneRowTabPillInset * scale;\n    }\n'
    '    const int bottom = top + tab_height;\n'))
edit(R, "chrome/browser/ui/views/tabs/common/BUILD.gn", lambda t: add_dep(t, 'source_set("impl") {', "//mast/common"))

# The suggestion list opens wider than the narrow address bar, growing left.
edit(R, "chrome/browser/ui/views/omnibox/omnibox_popup_presenter_base.cc", lambda t: once(once(t,
    '#include "chrome/browser/ui/views/omnibox/omnibox_popup_presenter_base.h"\n',
    '#include "chrome/browser/ui/views/omnibox/omnibox_popup_presenter_base.h"\n\n#include "mast/browser/ui/mast_one_row_layout.h"\n'),
    '    gfx::Rect widget_bounds = location_bar_->BoundsInScreen();\n',
    '    gfx::Rect widget_bounds = location_bar_->BoundsInScreen();\n'
    '    if (widget_->parent()) {  // Mast: the popup\'s parent is the browser window.\n'
    '      mast::WidenOmniboxPopup(widget_bounds,\n'
    '                              widget_->parent()->GetWindowBoundsInScreen());\n    }\n'))
edit(R, "chrome/browser/ui/views/omnibox/BUILD.gn", lambda t: add_dep(t, 'source_set("impl") {', "//mast/common"))

# The toolbar's one-row arrangement, once the window and its toolbar exist (the
# embedder hook Chromium leaves for forks).
edit(R, "chrome/browser/ui/browser_window/internal/embedder_browser_window_features.cc", lambda t: once(once(t,
    '#include "chrome/browser/ui/views/frame/browser_view.h"\n',
    '#include "chrome/browser/ui/views/frame/browser_view.h"\n#include "mast/browser/ui/mast_one_row_toolbar.h"\n'),
    'void EmbedderBrowserWindowFeatures::InitPostWindowConstruction(\n    BrowserWindowInterface* browser) {\n  // Embedder implementation goes here.\n}',
    'void EmbedderBrowserWindowFeatures::InitPostWindowConstruction(\n    BrowserWindowInterface* browser) {\n  mast::OnBrowserWindowReady(browser);\n}'))
edit(R, "chrome/browser/ui/browser_window/internal/BUILD.gn", lambda t: add_dep(once(t,
    '      "embedder_browser_window_features.cc",\n',
    '      "embedder_browser_window_features.cc",\n      "//mast/browser/ui/mast_one_row_toolbar.cc",\n      "//mast/browser/ui/mast_one_row_toolbar.h",\n'),
    'source_set("internal") {', "//mast/common"))

# No 1px line between the tab row and the page: in the one-row window the page meets the
# row directly (with a wallpaper the line cut the picture in two).
edit(R, "chrome/browser/ui/views/frame/layout/browser_view_tabbed_layout_impl.cc", lambda t: once(t,
    '  views().multi_contents_view->SetShouldShowTopSeparator(\n'
    '      separator_info.multi_contents_separator);\n',
    '  views().multi_contents_view->SetShouldShowTopSeparator(\n'
    '      separator_info.multi_contents_separator && !mast::IsOneRowWindow());\n'))

# On Customize's image the address field is translucent (mast_chrome_color_mixer.cc). Its
# textfield copies the field's colour as its own background, which would paint a darker
# rectangle inside the pill; with a translucent field it paints none.
edit(T, "chrome/browser/ui/views/location_bar/location_bar_view.cc", lambda t: once(t,
    '  omnibox_view_->SetBackgroundColor(background_color_);\n',
    '  // Mast: no second (darker) layer inside a translucent field.\n'
    '  omnibox_view_->SetBackgroundColor(\n'
    '      SkColorGetA(background_color_) == SK_AlphaOPAQUE ? background_color_\n'
    '                                                       : SK_ColorTRANSPARENT);\n'))

# Customize's image behind the tab row (mast/browser/ui/mast_wallpaper.h). The two places
# Chromium paints a theme's frame image: the frame view's GetFrameImage() (every platform's
# frame tiles it behind the tabs and window buttons) and ThemedBackground (the toolbar,
# which is in the row in the one-row window). Both align it to the same origin.
edit(R, "chrome/browser/ui/views/frame/browser_frame_view.cc", lambda t: once(once(t,
    '#include "chrome/grit/theme_resources.h"\n',
    '#include "chrome/grit/theme_resources.h"\n#include "mast/browser/ui/mast_wallpaper.h"\n'),
    'gfx::ImageSkia BrowserFrameView::GetFrameImage(\n    BrowserFrameActiveState active_state) const {\n',
    'gfx::ImageSkia BrowserFrameView::GetFrameImage(\n    BrowserFrameActiveState active_state) const {\n'
    '  // Mast: Customize\'s image continues behind the tab row.\n'
    '  if (gfx::ImageSkia wallpaper = mast::WallpaperFrameImage(GetBrowserView());\n'
    '      !wallpaper.isNull()) {\n    return wallpaper;\n  }\n'))
edit(R, "chrome/browser/ui/views/frame/themed_background.cc", lambda t: once(once(t,
    '#include "chrome/browser/ui/color/chrome_color_id.h"\n',
    '#include "chrome/browser/ui/color/chrome_color_id.h"\n#include "mast/browser/ui/mast_wallpaper.h"\n'),
    '                                       ThemeChoice theme_choice) {\n  bool painted =\n',
    '                                       ThemeChoice theme_choice) {\n'
    '  // Mast: the toolbar\'s part of the tab row shows Customize\'s image too.\n'
    '  if (mast::PaintWallpaperBackground(canvas, view, browser_view)) {\n    return;\n  }\n'
    '  bool painted =\n'))
edit(R, "chrome/browser/ui/views/frame/BUILD.gn", lambda t: add_dep(add_dep(add_dep(once(t,
    'source_set("themed_background_impl") {\n  sources = [ "themed_background.cc" ]\n',
    'source_set("themed_background_impl") {\n  sources = [\n    "themed_background.cc",\n'
    '    "//mast/browser/ui/mast_wallpaper.cc",\n    "//mast/browser/ui/mast_wallpaper.h",\n  ]\n'),
    'source_set("themed_background_impl") {', "//mast/common"),
    'source_set("themed_background_impl") {', "//ui/display"),
    'source_set("themed_background_impl") {', "//ui/gfx/codec"))

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

# The empty address bar says "Search": the one-row bar is narrow, and the engine's
# name isn't worth the room. (Other languages fall back to this English text.)
edit(B, "chrome/app/generated_resources.grd", lambda t: once(t,
    '        Search <ph name="SEARCH_ENGINE_NAME">$1<ex>Google</ex></ph> or type a URL\n      </message>\n      <if expr="not use_titlecase">',
    '        Search\n      </message>\n      <if expr="not use_titlecase">'))

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
