// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/shell/test_runner/accessibility_controller.h"

#include "base/macros.h"
#include "content/shell/test_runner/web_view_test_proxy.h"
#include "gin/handle.h"
#include "gin/object_template_builder.h"
#include "gin/wrappable.h"
#include "third_party/WebKit/public/web/WebDocument.h"
#include "third_party/WebKit/public/web/WebElement.h"
#include "third_party/WebKit/public/web/WebFrame.h"
#include "third_party/WebKit/public/web/WebKit.h"
#include "third_party/WebKit/public/web/WebLocalFrame.h"
#include "third_party/WebKit/public/web/WebSettings.h"
#include "third_party/WebKit/public/web/WebView.h"

namespace test_runner {

class AccessibilityControllerBindings
    : public gin::Wrappable<AccessibilityControllerBindings> {
 public:
  static gin::WrapperInfo kWrapperInfo;

  static void Install(base::WeakPtr<AccessibilityController> controller,
                      blink::WebLocalFrame* frame);

 private:
  explicit AccessibilityControllerBindings(
      base::WeakPtr<AccessibilityController> controller);
  ~AccessibilityControllerBindings() override;

  // gin::Wrappable:
  gin::ObjectTemplateBuilder GetObjectTemplateBuilder(
      v8::Isolate* isolate) override;

  void LogAccessibilityEvents();
  void SetNotificationListener(v8::Local<v8::Function> callback);
  void UnsetNotificationListener();
  v8::Local<v8::Object> FocusedElement();
  v8::Local<v8::Object> RootElement();
  v8::Local<v8::Object> AccessibleElementById(const std::string& id);

  base::WeakPtr<AccessibilityController> controller_;

  DISALLOW_COPY_AND_ASSIGN(AccessibilityControllerBindings);
};

gin::WrapperInfo AccessibilityControllerBindings::kWrapperInfo = {
    gin::kEmbedderNativeGin};

// static
void AccessibilityControllerBindings::Install(
    base::WeakPtr<AccessibilityController> controller,
    blink::WebLocalFrame* frame) {
  v8::Isolate* isolate = blink::mainThreadIsolate();
  v8::HandleScope handle_scope(isolate);
  v8::Local<v8::Context> context = frame->mainWorldScriptContext();
  if (context.IsEmpty())
    return;

  v8::Context::Scope context_scope(context);

  gin::Handle<AccessibilityControllerBindings> bindings = gin::CreateHandle(
      isolate, new AccessibilityControllerBindings(controller));
  if (bindings.IsEmpty())
    return;
  v8::Local<v8::Object> global = context->Global();
  global->Set(gin::StringToV8(isolate, "accessibilityController"),
              bindings.ToV8());
}

AccessibilityControllerBindings::AccessibilityControllerBindings(
    base::WeakPtr<AccessibilityController> controller)
    : controller_(controller) {}

AccessibilityControllerBindings::~AccessibilityControllerBindings() {}

gin::ObjectTemplateBuilder
AccessibilityControllerBindings::GetObjectTemplateBuilder(
    v8::Isolate* isolate) {
  return gin::Wrappable<
             AccessibilityControllerBindings>::GetObjectTemplateBuilder(isolate)
      .SetMethod("logAccessibilityEvents",
                 &AccessibilityControllerBindings::LogAccessibilityEvents)
      .SetMethod("setNotificationListener",
                 &AccessibilityControllerBindings::SetNotificationListener)
      .SetMethod("unsetNotificationListener",
                 &AccessibilityControllerBindings::UnsetNotificationListener)
      .SetProperty("focusedElement",
                   &AccessibilityControllerBindings::FocusedElement)
      .SetProperty("rootElement", &AccessibilityControllerBindings::RootElement)
      .SetMethod("accessibleElementById",
                 &AccessibilityControllerBindings::AccessibleElementById)
      // TODO(hajimehoshi): These are for backward compatibility. Remove them.
      .SetMethod("addNotificationListener",
                 &AccessibilityControllerBindings::SetNotificationListener)
      .SetMethod("removeNotificationListener",
                 &AccessibilityControllerBindings::UnsetNotificationListener);
}

void AccessibilityControllerBindings::LogAccessibilityEvents() {
  if (controller_)
    controller_->LogAccessibilityEvents();
}

void AccessibilityControllerBindings::SetNotificationListener(
    v8::Local<v8::Function> callback) {
  if (controller_)
    controller_->SetNotificationListener(callback);
}

void AccessibilityControllerBindings::UnsetNotificationListener() {
  if (controller_)
    controller_->UnsetNotificationListener();
}

v8::Local<v8::Object> AccessibilityControllerBindings::FocusedElement() {
  return controller_ ? controller_->FocusedElement() : v8::Local<v8::Object>();
}

v8::Local<v8::Object> AccessibilityControllerBindings::RootElement() {
  return controller_ ? controller_->RootElement() : v8::Local<v8::Object>();
}

v8::Local<v8::Object> AccessibilityControllerBindings::AccessibleElementById(
    const std::string& id) {
  return controller_ ? controller_->AccessibleElementById(id)
                     : v8::Local<v8::Object>();
}

AccessibilityController::AccessibilityController(
    WebViewTestProxyBase* web_view_test_proxy_base)
    : log_accessibility_events_(false),
      web_view_test_proxy_base_(web_view_test_proxy_base),
      weak_factory_(this) {}

AccessibilityController::~AccessibilityController() {}

void AccessibilityController::Reset() {
  elements_.Clear();
  notification_callback_.Reset();
  log_accessibility_events_ = false;
}

void AccessibilityController::Install(blink::WebLocalFrame* frame) {
  frame->view()->settings()->setAccessibilityEnabled(true);
  frame->view()->settings()->setInlineTextBoxAccessibilityEnabled(true);

  AccessibilityControllerBindings::Install(weak_factory_.GetWeakPtr(), frame);
}

bool AccessibilityController::ShouldLogAccessibilityEvents() {
  return log_accessibility_events_;
}

void AccessibilityController::NotificationReceived(
    const blink::WebAXObject& target,
    const std::string& notification_name) {
  v8::Isolate* isolate = blink::mainThreadIsolate();
  v8::HandleScope handle_scope(isolate);

  blink::WebFrame* frame = web_view()->mainFrame();
  if (!frame || frame->isWebRemoteFrame())
    return;

  v8::Local<v8::Context> context = frame->mainWorldScriptContext();
  if (context.IsEmpty())
    return;

  v8::Context::Scope context_scope(context);

  // Call notification listeners on the element.
  v8::Local<v8::Object> element_handle = elements_.GetOrCreate(target);
  if (element_handle.IsEmpty())
    return;

  WebAXObjectProxy* element;
  bool result = gin::ConvertFromV8(isolate, element_handle, &element);
  DCHECK(result);
  element->NotificationReceived(frame, notification_name);

  if (notification_callback_.IsEmpty())
    return;

  // Call global notification listeners.
  v8::Local<v8::Value> argv[] = {
      element_handle, v8::String::NewFromUtf8(isolate, notification_name.data(),
                                              v8::String::kNormalString,
                                              notification_name.size()),
  };
  frame->callFunctionEvenIfScriptDisabled(
      v8::Local<v8::Function>::New(isolate, notification_callback_),
      context->Global(), arraysize(argv), argv);
}

void AccessibilityController::LogAccessibilityEvents() {
  log_accessibility_events_ = true;
}

void AccessibilityController::SetNotificationListener(
    v8::Local<v8::Function> callback) {
  v8::Isolate* isolate = blink::mainThreadIsolate();
  notification_callback_.Reset(isolate, callback);
}

void AccessibilityController::UnsetNotificationListener() {
  notification_callback_.Reset();
}

v8::Local<v8::Object> AccessibilityController::FocusedElement() {
  blink::WebFrame* frame = web_view()->mainFrame();
  if (!frame)
    return v8::Local<v8::Object>();

  blink::WebAXObject focused_element =
      frame->document().focusedAccessibilityObject();
  if (focused_element.isNull())
    focused_element = web_view()->accessibilityObject();
  return elements_.GetOrCreate(focused_element);
}

v8::Local<v8::Object> AccessibilityController::RootElement() {
  blink::WebAXObject root_element = web_view()->accessibilityObject();
  return elements_.GetOrCreate(root_element);
}

v8::Local<v8::Object> AccessibilityController::AccessibleElementById(
    const std::string& id) {
  blink::WebAXObject root_element = web_view()->accessibilityObject();

  if (!root_element.updateLayoutAndCheckValidity())
    return v8::Local<v8::Object>();

  return FindAccessibleElementByIdRecursive(
      root_element, blink::WebString::fromUTF8(id.c_str()));
}

v8::Local<v8::Object>
AccessibilityController::FindAccessibleElementByIdRecursive(
    const blink::WebAXObject& obj,
    const blink::WebString& id) {
  if (obj.isNull() || obj.isDetached())
    return v8::Local<v8::Object>();

  blink::WebNode node = obj.node();
  if (!node.isNull() && node.isElementNode()) {
    blink::WebElement element = node.to<blink::WebElement>();
    element.getAttribute("id");
    if (element.getAttribute("id") == id)
      return elements_.GetOrCreate(obj);
  }

  unsigned childCount = obj.childCount();
  for (unsigned i = 0; i < childCount; i++) {
    v8::Local<v8::Object> result =
        FindAccessibleElementByIdRecursive(obj.childAt(i), id);
    if (*result)
      return result;
  }

  return v8::Local<v8::Object>();
}

blink::WebView* AccessibilityController::web_view() {
  return web_view_test_proxy_base_->web_view();
}

}  // namespace test_runner
