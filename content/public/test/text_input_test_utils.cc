// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/public/test/text_input_test_utils.h"

#include <unordered_set>

#include "content/browser/frame_host/render_frame_host_impl.h"
#include "content/browser/renderer_host/render_widget_host_impl.h"
#include "content/browser/renderer_host/render_widget_host_view_aura.h"
#include "content/browser/renderer_host/render_widget_host_view_base.h"
#include "content/browser/renderer_host/render_widget_host_view_base_observer.h"
#include "content/browser/renderer_host/text_input_manager.h"
#include "content/browser/web_contents/web_contents_impl.h"
#include "content/common/input_messages.h"
#include "content/common/text_input_state.h"
#include "content/common/view_messages.h"
#include "content/public/browser/render_widget_host_view.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/test/test_utils.h"
#include "third_party/WebKit/public/web/WebCompositionUnderline.h"
#include "ui/base/ime/composition_underline.h"
#include "ui/base/ime/input_method.h"
#include "ui/base/ime/input_method_observer.h"

namespace ui {
class TextInputClient;
}

namespace content {

// This class is an observer of TextInputManager associated with the provided
// WebContents. An instance of this class is used in TextInputManagerTester to
// expose the required API for testing outside of content/.
class TextInputManagerTester::InternalObserver
    : public TextInputManager::Observer,
      public WebContentsObserver {
 public:
  InternalObserver(WebContents* web_contents, TextInputManagerTester* tester)
      : WebContentsObserver(web_contents),
        updated_view_(nullptr),
        text_input_state_changed_(false) {
    text_input_manager_ =
        static_cast<WebContentsImpl*>(web_contents)->GetTextInputManager();
    DCHECK(!!text_input_manager_);
    text_input_manager_->AddObserver(this);
  }

  ~InternalObserver() override {
    if (text_input_manager_)
      text_input_manager_->RemoveObserver(this);
  }

  void set_update_text_input_state_called_callback(
      const base::Closure& callback) {
    update_text_input_state_callback_ = callback;
  }

  void set_on_selection_bounds_changed_callback(const base::Closure& callback) {
    on_selection_bounds_changed_callback_ = callback;
  }

  void set_on_ime_composition_range_changed_callback(
      const base::Closure& callback) {
    on_ime_composition_range_changed_callback_ = callback;
  }

  void set_on_text_selection_changed_callback(const base::Closure& callback) {
    on_text_selection_changed_callback_ = callback;
  }

  RenderWidgetHostView* GetUpdatedView() const { return updated_view_; }

  bool text_input_state_changed() const { return text_input_state_changed_; }

  TextInputManager* text_input_manager() const { return text_input_manager_; }

  // TextInputManager::Observer implementations.
  void OnUpdateTextInputStateCalled(TextInputManager* text_input_manager,
                                    RenderWidgetHostViewBase* updated_view,
                                    bool did_change_state) override {
    if (text_input_manager_ != text_input_manager)
      return;
    text_input_state_changed_ = did_change_state;
    updated_view_ = updated_view;
    if (!update_text_input_state_callback_.is_null())
      update_text_input_state_callback_.Run();
  }

  void OnSelectionBoundsChanged(
      TextInputManager* text_input_manager_,
      RenderWidgetHostViewBase* updated_view) override {
    updated_view_ = updated_view;
    if (!on_selection_bounds_changed_callback_.is_null())
      on_selection_bounds_changed_callback_.Run();
  }

  void OnImeCompositionRangeChanged(
      TextInputManager* text_input_manager,
      RenderWidgetHostViewBase* updated_view) override {
    updated_view_ = updated_view;
    if (!on_ime_composition_range_changed_callback_.is_null())
      on_ime_composition_range_changed_callback_.Run();
  }

  void OnTextSelectionChanged(TextInputManager* text_input_manager,
                              RenderWidgetHostViewBase* updated_view) override {
    updated_view_ = updated_view;
    if (!on_text_selection_changed_callback_.is_null())
      on_text_selection_changed_callback_.Run();
  }

  // WebContentsObserver implementation.
  void WebContentsDestroyed() override { text_input_manager_ = nullptr; }

 private:
  TextInputManager* text_input_manager_;
  RenderWidgetHostViewBase* updated_view_;
  bool text_input_state_changed_;
  base::Closure update_text_input_state_callback_;
  base::Closure on_selection_bounds_changed_callback_;
  base::Closure on_ime_composition_range_changed_callback_;
  base::Closure on_text_selection_changed_callback_;

  DISALLOW_COPY_AND_ASSIGN(InternalObserver);
};

// This class observes the lifetime of a RenderWidgetHostView. An instance of
// this class is used in TestRenderWidgetHostViewDestructionObserver to expose
// the required observer API for testing outside of content/.
class TestRenderWidgetHostViewDestructionObserver::InternalObserver
    : public RenderWidgetHostViewBaseObserver {
 public:
  InternalObserver(RenderWidgetHostViewBase* view)
      : view_(view), destroyed_(false) {
    view->AddObserver(this);
  }

  ~InternalObserver() override {
    if (view_)
      view_->RemoveObserver(this);
  }

  void Wait() {
    if (destroyed_)
      return;
    message_loop_runner_ = new content::MessageLoopRunner();
    message_loop_runner_->Run();
  }

 private:
  void OnRenderWidgetHostViewBaseDestroyed(
      RenderWidgetHostViewBase* view) override {
    DCHECK_EQ(view_, view);
    destroyed_ = true;
    view->RemoveObserver(this);
    view_ = nullptr;
    if (message_loop_runner_)
      message_loop_runner_->Quit();
  }

  RenderWidgetHostViewBase* view_;
  bool destroyed_;
  scoped_refptr<MessageLoopRunner> message_loop_runner_;

  DISALLOW_COPY_AND_ASSIGN(InternalObserver);
};

#ifdef USE_AURA
class InputMethodObserverAura : public TestInputMethodObserver,
                                public ui::InputMethodObserver {
 public:
  explicit InputMethodObserverAura(ui::InputMethod* input_method)
      : input_method_(input_method), text_input_client_(nullptr) {
    input_method_->AddObserver(this);
  }

  ~InputMethodObserverAura() override {
    if (input_method_)
      input_method_->RemoveObserver(this);
  }

  // TestInputMethodObserver implementations.
  ui::TextInputType GetTextInputTypeFromClient() override {
    if (text_input_client_)
      return text_input_client_->GetTextInputType();

    return ui::TEXT_INPUT_TYPE_NONE;
  }

  void SetOnTextInputTypeChangedCallback(
      const base::Closure& callback) override {
    on_text_input_type_changed_callback_ = callback;
  }

  void SetOnShowImeIfNeededCallback(const base::Closure& callback) override {
    on_show_ime_if_needed_callback_ = callback;
  }

 private:
  // ui::InputMethodObserver implementations.
  void OnTextInputTypeChanged(const ui::TextInputClient* client) override {
    text_input_client_ = client;
    on_text_input_type_changed_callback_.Run();
  }

  void OnFocus() override {}
  void OnBlur() override {}
  void OnCaretBoundsChanged(const ui::TextInputClient* client) override {}
  void OnTextInputStateChanged(const ui::TextInputClient* client) override {}
  void OnInputMethodDestroyed(const ui::InputMethod* input_method) override {}

  void OnShowImeIfNeeded() override { on_show_ime_if_needed_callback_.Run(); }

  ui::InputMethod* input_method_;
  const ui::TextInputClient* text_input_client_;
  base::Closure on_text_input_type_changed_callback_;
  base::Closure on_show_ime_if_needed_callback_;

  DISALLOW_COPY_AND_ASSIGN(InputMethodObserverAura);
};
#endif

ui::TextInputType GetTextInputTypeFromWebContents(WebContents* web_contents) {
  const TextInputState* state = static_cast<WebContentsImpl*>(web_contents)
                                    ->GetTextInputManager()
                                    ->GetTextInputState();
  return !!state ? state->type : ui::TEXT_INPUT_TYPE_NONE;
}

bool GetTextInputTypeForView(WebContents* web_contents,
                             RenderWidgetHostView* view,
                             ui::TextInputType* type) {
  TextInputManager* manager =
      static_cast<WebContentsImpl*>(web_contents)->GetTextInputManager();

  RenderWidgetHostViewBase* view_base =
      static_cast<RenderWidgetHostViewBase*>(view);
  if (!manager || !manager->IsRegistered(view_base))
    return false;

  *type = manager->GetTextInputTypeForViewForTesting(view_base);

  return true;
}

bool RequestCompositionInfoFromActiveWidget(WebContents* web_contents) {
  TextInputManager* manager =
      static_cast<WebContentsImpl*>(web_contents)->GetTextInputManager();
  if (!manager || !manager->GetActiveWidget())
    return false;

  manager->GetActiveWidget()->Send(new InputMsg_RequestCompositionUpdate(
      manager->GetActiveWidget()->GetRoutingID(), true, false));
  return true;
}

bool DoesFrameHaveFocusedEditableElement(RenderFrameHost* frame) {
  return static_cast<RenderFrameHostImpl*>(frame)
      ->has_focused_editable_element();
}

void SendImeCommitTextToWidget(
    RenderWidgetHost* rwh,
    const base::string16& text,
    const std::vector<ui::CompositionUnderline>& underlines,
    const gfx::Range& replacement_range,
    int relative_cursor_pos) {
  std::vector<blink::WebCompositionUnderline> web_composition_underlines;
  for (auto underline : underlines) {
    web_composition_underlines.emplace_back(
        static_cast<int>(underline.start_offset),
        static_cast<int>(underline.end_offset), underline.color,
        underline.thick, underline.background_color);
  }
  RenderWidgetHostImpl::From(rwh)->ImeCommitText(
      text, web_composition_underlines, replacement_range, relative_cursor_pos);
}

size_t GetRegisteredViewsCountFromTextInputManager(WebContents* web_contents) {
  std::unordered_set<RenderWidgetHostView*> views;
  TextInputManager* manager =
      static_cast<WebContentsImpl*>(web_contents)->GetTextInputManager();
  return !!manager ? manager->GetRegisteredViewsCountForTesting() : 0;
}

RenderWidgetHostView* GetActiveViewFromWebContents(WebContents* web_contents) {
  return static_cast<WebContentsImpl*>(web_contents)
      ->GetTextInputManager()
      ->active_view_for_testing();
}

TextInputManagerTester::TextInputManagerTester(WebContents* web_contents)
    : observer_(new InternalObserver(web_contents, this)) {}

TextInputManagerTester::~TextInputManagerTester() {}

void TextInputManagerTester::SetUpdateTextInputStateCalledCallback(
    const base::Closure& callback) {
  observer_->set_update_text_input_state_called_callback(callback);
}

void TextInputManagerTester::SetOnSelectionBoundsChangedCallback(
    const base::Closure& callback) {
  observer_->set_on_selection_bounds_changed_callback(callback);
}

void TextInputManagerTester::SetOnImeCompositionRangeChangedCallback(
    const base::Closure& callback) {
  observer_->set_on_ime_composition_range_changed_callback(callback);
}

void TextInputManagerTester::SetOnTextSelectionChangedCallback(
    const base::Closure& callback) {
  observer_->set_on_text_selection_changed_callback(callback);
}

bool TextInputManagerTester::GetTextInputType(ui::TextInputType* type) {
  DCHECK(observer_->text_input_manager());
  const TextInputState* state =
      observer_->text_input_manager()->GetTextInputState();
  if (!state)
    return false;
  *type = state->type;
  return true;
}

bool TextInputManagerTester::GetTextInputValue(std::string* value) {
  DCHECK(observer_->text_input_manager());
  const TextInputState* state =
      observer_->text_input_manager()->GetTextInputState();
  if (!state)
    return false;
  *value = state->value;
  return true;
}

const RenderWidgetHostView* TextInputManagerTester::GetActiveView() {
  DCHECK(observer_->text_input_manager());
  return observer_->text_input_manager()->active_view_for_testing();
}

RenderWidgetHostView* TextInputManagerTester::GetUpdatedView() {
  return observer_->GetUpdatedView();
}

bool TextInputManagerTester::GetCurrentTextSelectionLength(size_t* length) {
  DCHECK(observer_->text_input_manager());

  if (!observer_->text_input_manager()->GetActiveWidget())
    return false;

  *length = observer_->text_input_manager()->GetTextSelection()->text().size();
  return true;
}

bool TextInputManagerTester::IsTextInputStateChanged() {
  return observer_->text_input_state_changed();
}

TestRenderWidgetHostViewDestructionObserver::
    TestRenderWidgetHostViewDestructionObserver(RenderWidgetHostView* view)
    : observer_(
          new InternalObserver(static_cast<RenderWidgetHostViewBase*>(view))) {}

TestRenderWidgetHostViewDestructionObserver::
    ~TestRenderWidgetHostViewDestructionObserver() {}

void TestRenderWidgetHostViewDestructionObserver::Wait() {
  observer_->Wait();
}

TextInputStateSender::TextInputStateSender(RenderWidgetHostView* view)
    : text_input_state_(new TextInputState()),
      view_(static_cast<RenderWidgetHostViewBase*>(view)) {}

TextInputStateSender::~TextInputStateSender() {}

void TextInputStateSender::Send() {
  if (view_)
    view_->TextInputStateChanged(*text_input_state_);
}

void TextInputStateSender::SetFromCurrentState() {
  if (view_) {
    *text_input_state_ =
        *RenderWidgetHostImpl::From(view_->GetRenderWidgetHost())
             ->delegate()
             ->GetTextInputManager()
             ->GetTextInputState();
  }
}

void TextInputStateSender::SetType(ui::TextInputType type) {
  text_input_state_->type = type;
}

void TextInputStateSender::SetMode(ui::TextInputMode mode) {
  text_input_state_->mode = mode;
}

void TextInputStateSender::SetFlags(int flags) {
  text_input_state_->flags = flags;
}

void TextInputStateSender::SetCanComposeInline(bool can_compose_inline) {
  text_input_state_->can_compose_inline = can_compose_inline;
}

void TextInputStateSender::SetShowImeIfNeeded(bool show_ime_if_needed) {
  text_input_state_->show_ime_if_needed = show_ime_if_needed;
}

TestInputMethodObserver::TestInputMethodObserver() {}

TestInputMethodObserver::~TestInputMethodObserver() {}

// static
std::unique_ptr<TestInputMethodObserver> TestInputMethodObserver::Create(
    WebContents* web_contents) {
  std::unique_ptr<TestInputMethodObserver> observer;

#ifdef USE_AURA
  RenderWidgetHostViewAura* view = static_cast<RenderWidgetHostViewAura*>(
      web_contents->GetRenderWidgetHostView());
  observer.reset(new InputMethodObserverAura(view->GetInputMethod()));
#endif
  return observer;
}

}  // namespace content
