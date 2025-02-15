/*
 * Copyright (C) 2010 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "bindings/core/v8/V8PopStateEvent.h"

#include "bindings/core/v8/SerializedScriptValue.h"
#include "bindings/core/v8/SerializedScriptValueFactory.h"
#include "bindings/core/v8/V8HiddenValue.h"
#include "bindings/core/v8/V8History.h"
#include "core/events/PopStateEvent.h"
#include "core/frame/History.h"

namespace blink {

// Save the state value to a hidden attribute in the V8PopStateEvent, and return
// it, for convenience.
static v8::Local<v8::Value> cacheState(ScriptState* scriptState,
                                       v8::Local<v8::Object> popStateEvent,
                                       v8::Local<v8::Value> state) {
  V8HiddenValue::setHiddenValue(scriptState, popStateEvent,
                                V8HiddenValue::state(scriptState->isolate()),
                                state);
  return state;
}

void V8PopStateEvent::stateAttributeGetterCustom(
    const v8::FunctionCallbackInfo<v8::Value>& info) {
  v8::Isolate* isolate = info.GetIsolate();
  ScriptState* scriptState = ScriptState::current(isolate);
  v8::Local<v8::Value> result = V8HiddenValue::getHiddenValue(
      scriptState, info.Holder(), V8HiddenValue::state(isolate));

  if (!result.IsEmpty()) {
    v8SetReturnValue(info, result);
    return;
  }

  PopStateEvent* event = V8PopStateEvent::toImpl(info.Holder());
  History* history = event->history();
  if (!history || !event->serializedState()) {
    // If the event doesn't have serializedState(), it means that the
    // event was initialized with PopStateEventInit. In such case, we need
    // to get a v8 value for the current world from state().
    if (event->serializedState())
      result = event->serializedState()->deserialize(isolate);
    else
      result = event->state().v8ValueFor(scriptState);
    if (result.IsEmpty())
      result = v8::Null(isolate);
    v8SetReturnValue(info, cacheState(scriptState, info.Holder(), result));
    return;
  }

  // There's no cached value from a previous invocation, nor a state value was
  // provided by the event, but there is a history object, so first we need to
  // see if the state object has been deserialized through the history object
  // already.
  // The current history state object might've changed in the meantime, so we
  // need to take care of using the correct one, and always share the same
  // deserialization with history.state.

  bool isSameState = history->isSameAsCurrentState(event->serializedState());

  if (isSameState) {
    v8::Local<v8::Value> v8HistoryValue = ToV8(history, info.Holder(), isolate);
    if (v8HistoryValue.IsEmpty())
      return;
    v8::Local<v8::Object> v8History = v8HistoryValue.As<v8::Object>();
    if (!history->stateChanged()) {
      result = V8HiddenValue::getHiddenValue(scriptState, v8History,
                                             V8HiddenValue::state(isolate));
      if (!result.IsEmpty()) {
        v8SetReturnValue(info, cacheState(scriptState, info.Holder(), result));
        return;
      }
    }
    result = event->serializedState()->deserialize(isolate);
    V8HiddenValue::setHiddenValue(scriptState, v8History,
                                  V8HiddenValue::state(isolate), result);
  } else {
    result = event->serializedState()->deserialize(isolate);
  }

  v8SetReturnValue(info, cacheState(scriptState, info.Holder(), result));
}

}  // namespace blink
