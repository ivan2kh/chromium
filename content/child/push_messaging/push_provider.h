// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_CHILD_PUSH_MESSAGING_PUSH_PROVIDER_H_
#define CONTENT_CHILD_PUSH_MESSAGING_PUSH_PROVIDER_H_

#include <stdint.h>

#include <memory>
#include <string>
#include <vector>

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "content/common/push_messaging.mojom.h"
#include "content/public/child/worker_thread.h"
#include "content/public/common/push_messaging_status.h"
#include "third_party/WebKit/public/platform/modules/push_messaging/WebPushError.h"
#include "third_party/WebKit/public/platform/modules/push_messaging/WebPushProvider.h"

class GURL;

namespace blink {
struct WebPushSubscriptionOptions;
}

namespace content {

struct PushSubscriptionOptions;

blink::WebPushError PushRegistrationStatusToWebPushError(
    PushRegistrationStatus status);

class PushProvider : public blink::WebPushProvider,
                     public WorkerThread::Observer {
 public:
  ~PushProvider() override;

  static PushProvider* ThreadSpecificInstance(
      const scoped_refptr<base::SingleThreadTaskRunner>&
          main_thread_task_runner);

  // WorkerThread::Observer implementation.
  void WillStopCurrentWorkerThread() override;

  // blink::WebPushProvider implementation.
  void subscribe(
      blink::WebServiceWorkerRegistration* service_worker_registration,
      const blink::WebPushSubscriptionOptions& options,
      std::unique_ptr<blink::WebPushSubscriptionCallbacks> callbacks) override;
  void unsubscribe(
      blink::WebServiceWorkerRegistration* service_worker_registration,
      std::unique_ptr<blink::WebPushUnsubscribeCallbacks> callbacks) override;
  void getSubscription(
      blink::WebServiceWorkerRegistration* service_worker_registration,
      std::unique_ptr<blink::WebPushSubscriptionCallbacks> callbacks) override;
  void getPermissionStatus(
      blink::WebServiceWorkerRegistration* service_worker_registration,
      const blink::WebPushSubscriptionOptions& options,
      std::unique_ptr<blink::WebPushPermissionStatusCallbacks> callbacks)
      override;

 private:
  explicit PushProvider(const scoped_refptr<base::SingleThreadTaskRunner>&
                            main_thread_task_runner);

  static void GetInterface(mojom::PushMessagingRequest request);

  void DidSubscribe(
      std::unique_ptr<blink::WebPushSubscriptionCallbacks> callbacks,
      content::PushRegistrationStatus status,
      const base::Optional<GURL>& endpoint,
      const base::Optional<content::PushSubscriptionOptions>& options,
      const base::Optional<std::vector<uint8_t>>& p256dh,
      const base::Optional<std::vector<uint8_t>>& auth);

  void DidUnsubscribe(
      std::unique_ptr<blink::WebPushUnsubscribeCallbacks> callbacks,
      blink::WebPushError::ErrorType error_type,
      bool did_unsubscribe,
      const base::Optional<std::string>& error_message);

  void DidGetSubscription(
      std::unique_ptr<blink::WebPushSubscriptionCallbacks> callbacks,
      content::PushGetRegistrationStatus status,
      const base::Optional<GURL>& endpoint,
      const base::Optional<content::PushSubscriptionOptions>& options,
      const base::Optional<std::vector<uint8_t>>& p256dh,
      const base::Optional<std::vector<uint8_t>>& auth);

  void DidGetPermissionStatus(
      std::unique_ptr<blink::WebPushPermissionStatusCallbacks> callbacks,
      blink::WebPushError::ErrorType error_type,
      blink::WebPushPermissionStatus status);

  mojom::PushMessagingPtr push_messaging_manager_;

  DISALLOW_COPY_AND_ASSIGN(PushProvider);
};

}  // namespace content

#endif  // CONTENT_CHILD_PUSH_MESSAGING_PUSH_PROVIDER_H_
