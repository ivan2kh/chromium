// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef InstalledAppController_h
#define InstalledAppController_h

#include "core/dom/ContextLifecycleObserver.h"
#include "core/frame/LocalFrame.h"
#include "modules/ModulesExport.h"
#include "platform/Supplementable.h"
#include "public/platform/WebVector.h"
#include "public/platform/modules/installedapp/WebRelatedApplication.h"
#include "public/platform/modules/installedapp/WebRelatedAppsFetcher.h"
#include "public/platform/modules/installedapp/installed_app_provider.mojom-blink.h"
#include "public/platform/modules/installedapp/related_application.mojom-blink.h"
#include "wtf/RefPtr.h"
#include "wtf/Vector.h"

#include <memory>

namespace blink {

class MODULES_EXPORT InstalledAppController final
    : public GarbageCollectedFinalized<InstalledAppController>,
      public Supplement<LocalFrame>,
      public ContextLifecycleObserver {
  USING_GARBAGE_COLLECTED_MIXIN(InstalledAppController);
  WTF_MAKE_NONCOPYABLE(InstalledAppController);

 public:
  virtual ~InstalledAppController();

  // Gets a list of related apps from the current page's manifest that belong
  // to the current underlying platform, and are installed.
  void getInstalledRelatedApps(std::unique_ptr<AppInstalledCallbacks>);

  static void provideTo(LocalFrame&, WebRelatedAppsFetcher*);
  static InstalledAppController* from(LocalFrame&);
  static const char* supplementName();

  DECLARE_VIRTUAL_TRACE();

 private:
  class GetRelatedAppsCallbacks;

  InstalledAppController(LocalFrame&, WebRelatedAppsFetcher*);

  // Inherited from ContextLifecycleObserver.
  void contextDestroyed(ExecutionContext*) override;

  // Takes a set of related applications and filters them by those which belong
  // to the current underlying platform, and are actually installed and related
  // to the current page's origin. Passes the filtered list to the callback.
  void filterByInstalledApps(const WebVector<WebRelatedApplication>&,
                             std::unique_ptr<AppInstalledCallbacks>);

  // Callback from the InstalledAppProvider mojo service.
  void OnFilterInstalledApps(std::unique_ptr<blink::AppInstalledCallbacks>,
                             WTF::Vector<mojom::blink::RelatedApplicationPtr>);

  // Handle to the InstalledApp mojo service.
  mojom::blink::InstalledAppProviderPtr m_provider;

  WebRelatedAppsFetcher* m_relatedAppsFetcher;
};

}  // namespace blink

#endif  // InstalledAppController_h
