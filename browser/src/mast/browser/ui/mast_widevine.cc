// Copyright 2026 The Mast Authors

#include "mast/browser/ui/mast_widevine.h"

#include <memory>
#include <string>
#include <utility>

#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/functional/bind.h"
#include "base/memory/weak_ptr.h"
#include "base/no_destructor.h"
#include "base/path_service.h"
#include "base/task/thread_pool.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/infobars/confirm_infobar_creator.h"
#include "chrome/browser/lifetime/application_lifetime.h"
#include "chrome/common/chrome_paths.h"
#include "components/infobars/content/content_infobar_manager.h"
#include "components/infobars/core/confirm_infobar_delegate.h"
#include "components/infobars/core/infobar.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/web_contents.h"
#include "mast/common/mast_widevine_state.h"
#include "third_party/widevine/cdm/buildflags.h"

namespace mast {

namespace {

// This launch's Widevine: registered at startup (the hint file was there),
// downloaded since (needs a restart), or being downloaded.
bool loaded = false;
bool ready = false;
bool installing = false;

PrefService* LocalState() {
  return g_browser_process ? g_browser_process->local_state() : nullptr;
}

int Choice() {
  PrefService* p = LocalState();
  return p ? p->GetInteger(kWidevinePref) : kWidevineAsk;
}

void SetChoice(int choice) {
  if (PrefService* p = LocalState()) {
    p->SetInteger(kWidevinePref, choice);
  }
}

void Install(base::WeakPtr<content::WebContents> contents);

// One message in the tab, the only one of its kind there.
class Bar : public ConfirmInfoBarDelegate {
 public:
  enum class Kind { kAsk, kReady, kFailed };
  explicit Bar(Kind kind, base::WeakPtr<content::WebContents> contents)
      : kind_(kind), contents_(std::move(contents)) {}

  // ConfirmInfoBarDelegate:
  infobars::InfoBarDelegate::InfoBarIdentifier GetIdentifier() const override {
    return MAST_WIDEVINE_INFOBAR_DELEGATE;
  }
  std::u16string GetMessageText() const override {
    switch (kind_) {
      case Kind::kAsk:
        return u"This site plays protected audio or video, which needs "
               u"Google Widevine. Mast can download it from Google.";
      case Kind::kReady:
        return u"Widevine is ready. Restart Mast to play.";
      case Kind::kFailed:
        return u"Widevine couldn't be downloaded. Reload the page to try "
               u"again.";
    }
  }
  int GetButtons() const override {
    switch (kind_) {
      case Kind::kAsk:
        return BUTTON_OK | BUTTON_CANCEL;
      case Kind::kReady:
        return BUTTON_OK;
      case Kind::kFailed:
        return BUTTON_NONE;
    }
  }
  std::u16string GetButtonLabel(InfoBarButton button) const override {
    if (kind_ == Kind::kReady) {
      return u"Restart";
    }
    return button == BUTTON_OK ? u"Download" : u"Don't ask again";
  }
  bool Accept() override {
    if (kind_ == Kind::kReady) {
      chrome::AttemptRelaunch();  // tabs come back after the restart
      return true;
    }
    SetChoice(kWidevineOn);
    Install(contents_);
    return true;
  }
  bool Cancel() override {
    SetChoice(kWidevineOff);  // the close button only hides it for now
    return true;
  }
  bool EqualsDelegate(infobars::InfoBarDelegate* delegate) const override {
    return delegate->GetIdentifier() == GetIdentifier();
  }
  // Pages like Spotify change their address as you go: the message stays.
  bool ShouldExpire(const NavigationDetails& details) const override {
    return false;
  }

 private:
  const Kind kind_;
  const base::WeakPtr<content::WebContents> contents_;
};

void ShowInfoBar(content::WebContents* contents, Bar::Kind kind) {
  auto* manager =
      contents ? infobars::ContentInfoBarManager::FromWebContents(contents)
               : nullptr;
  if (!manager) {
    return;
  }
  // A new message replaces Mast's one before it in this tab.
  manager->AddInfoBar(
      CreateConfirmInfoBar(std::make_unique<Bar>(kind, contents->GetWeakPtr())),
      /*replace_existing=*/true);
}

void Install(base::WeakPtr<content::WebContents> contents) {
  if (installing) {
    return;
  }
  installing = true;
  const bool started = InstallWidevine(base::BindOnce(
      [](base::WeakPtr<content::WebContents> contents, bool ok) {
        installing = false;
        ready = ok;
        if (!ok) {
          SetChoice(kWidevineAsk);  // ask again next time
        }
        if (contents) {
          ShowInfoBar(contents.get(),
                      ok ? Bar::Kind::kReady : Bar::Kind::kFailed);
        }
      },
      std::move(contents)));
  if (!started) {
    installing = false;
  }
}

}  // namespace

// static
void MastWidevine::Start() {
  static base::NoDestructor<MastWidevine> instance;
}

MastWidevine::MastWidevine() {
  subscription_ = OnProtectedMediaRequested(base::BindRepeating(
      &MastWidevine::OnProtectedMedia, base::Unretained(this)));
#if BUILDFLAG(ENABLE_WIDEVINE_CDM_COMPONENT)
  // Whether this launch loaded Widevine: the hint file it reads at startup.
  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE, {base::MayBlock(), base::TaskPriority::BEST_EFFORT},
      base::BindOnce([] {
        base::FilePath hint;
        return base::PathService::Get(chrome::FILE_COMPONENT_WIDEVINE_CDM_HINT,
                                      &hint) &&
               base::PathExists(hint);
      }),
      base::BindOnce([](bool exists) { loaded = exists; }));
#endif
}

MastWidevine::~MastWidevine() = default;

void MastWidevine::OnProtectedMedia(const void* web_contents) {
  // Called while the page is alive (KeySystemSupportImpl::SetObserver).
  auto* contents =
      static_cast<content::WebContents*>(const_cast<void*>(web_contents));
  if (loaded || Choice() == kWidevineOff) {
    return;
  }
  if (ready) {
    ShowInfoBar(contents, Bar::Kind::kReady);
  } else if (Choice() == kWidevineAsk) {
    ShowInfoBar(contents, Bar::Kind::kAsk);
  } else {
    // Said yes before, but it isn't here yet (offline then): get it now.
    Install(contents->GetWeakPtr());
  }
}

}  // namespace mast
