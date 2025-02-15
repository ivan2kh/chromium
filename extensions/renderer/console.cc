// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/renderer/console.h"

#include "base/compiler_specific.h"
#include "base/debug/alias.h"
#include "base/lazy_instance.h"
#include "base/macros.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "base/strings/utf_string_conversions.h"
#include "content/public/child/worker_thread.h"
#include "content/public/renderer/render_frame.h"
#include "extensions/renderer/extension_frame_helper.h"
#include "extensions/renderer/script_context.h"
#include "extensions/renderer/script_context_set.h"
#include "extensions/renderer/v8_helpers.h"
#include "gin/converter.h"
#include "gin/per_isolate_data.h"

namespace extensions {
namespace console {

using namespace v8_helpers;

namespace {

// Writes |message| to stack to show up in minidump, then crashes.
void CheckWithMinidump(const std::string& message) {
  char minidump[1024];
  base::debug::Alias(&minidump);
  base::snprintf(
      minidump, arraysize(minidump), "e::console: %s", message.c_str());
  CHECK(false) << message;
}

void BoundLogMethodCallback(const v8::FunctionCallbackInfo<v8::Value>& info) {
  std::string message;
  for (int i = 0; i < info.Length(); ++i) {
    if (i > 0)
      message += " ";
    message += *v8::String::Utf8Value(info[i]);
  }

  // A worker's ScriptContext neither lives in ScriptContextSet nor it has a
  // RenderFrame associated with it, so early exit in this case.
  // TODO(lazyboy): Fix.
  if (content::WorkerThread::GetCurrentId() > 0)
    return;

  v8::Local<v8::Context> context = info.GetIsolate()->GetCurrentContext();
  ScriptContext* script_context =
      ScriptContextSet::GetContextByV8Context(context);
  // TODO(devlin): Consider (D)CHECK(script_context)
  content::RenderFrame* render_frame =
      script_context ? script_context->GetRenderFrame() : nullptr;
  const auto level = static_cast<content::ConsoleMessageLevel>(
      info.Data().As<v8::Int32>()->Value());
  AddMessage(render_frame, level, message);
}

gin::WrapperInfo kWrapperInfo = {gin::kEmbedderNativeGin};

}  // namespace

void Fatal(content::RenderFrame* render_frame, const std::string& message) {
  AddMessage(render_frame, content::CONSOLE_MESSAGE_LEVEL_ERROR, message);
  CheckWithMinidump(message);
}

void AddMessage(content::RenderFrame* render_frame,
                content::ConsoleMessageLevel level,
                const std::string& message) {
  if (!render_frame) {
    LOG(WARNING) << "Could not log \"" << message
                 << "\": no render frame found";
  } else {
    render_frame->AddMessageToConsole(level, message);
  }
}

v8::Local<v8::Object> AsV8Object(v8::Isolate* isolate) {
  v8::EscapableHandleScope handle_scope(isolate);
  gin::PerIsolateData* data = gin::PerIsolateData::From(isolate);
  v8::Local<v8::ObjectTemplate> templ = data->GetObjectTemplate(&kWrapperInfo);
  if (templ.IsEmpty()) {
    templ = v8::ObjectTemplate::New(isolate);
    static const struct {
      const char* name;
      content::ConsoleMessageLevel level;
    } methods[] = {
        {"debug", content::CONSOLE_MESSAGE_LEVEL_VERBOSE},
        {"log", content::CONSOLE_MESSAGE_LEVEL_INFO},
        {"warn", content::CONSOLE_MESSAGE_LEVEL_WARNING},
        {"error", content::CONSOLE_MESSAGE_LEVEL_ERROR},
    };
    for (const auto& method : methods) {
      v8::Local<v8::FunctionTemplate> function =
          v8::FunctionTemplate::New(isolate, BoundLogMethodCallback,
                                    v8::Integer::New(isolate, method.level));
      function->RemovePrototype();
      templ->Set(gin::StringToSymbol(isolate, method.name), function);
    }
    data->SetObjectTemplate(&kWrapperInfo, templ);
  }
  return handle_scope.Escape(
      templ->NewInstance(isolate->GetCurrentContext()).ToLocalChecked());
}

}  // namespace console
}  // namespace extensions
