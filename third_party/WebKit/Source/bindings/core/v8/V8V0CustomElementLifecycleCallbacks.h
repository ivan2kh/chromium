/*
 * Copyright (C) 2013 Google Inc. All rights reserved.
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

#ifndef V8V0CustomElementLifecycleCallbacks_h
#define V8V0CustomElementLifecycleCallbacks_h

#include <memory>

#include "bindings/core/v8/ScopedPersistent.h"
#include "bindings/core/v8/ScriptState.h"
#include "core/dom/custom/V0CustomElementLifecycleCallbacks.h"
#include "v8/include/v8.h"
#include "wtf/PassRefPtr.h"

namespace blink {

class V0CustomElementLifecycleCallbacks;
class Element;
class V8PerContextData;

class V8V0CustomElementLifecycleCallbacks final
    : public V0CustomElementLifecycleCallbacks {
 public:
  static V8V0CustomElementLifecycleCallbacks* create(
      ScriptState*,
      v8::Local<v8::Object> prototype,
      v8::MaybeLocal<v8::Function> created,
      v8::MaybeLocal<v8::Function> attached,
      v8::MaybeLocal<v8::Function> detached,
      v8::MaybeLocal<v8::Function> attributeChanged);

  ~V8V0CustomElementLifecycleCallbacks() override;

  bool setBinding(std::unique_ptr<V0CustomElementBinding>);

  DECLARE_VIRTUAL_TRACE();

 private:
  V8V0CustomElementLifecycleCallbacks(
      ScriptState*,
      v8::Local<v8::Object> prototype,
      v8::MaybeLocal<v8::Function> created,
      v8::MaybeLocal<v8::Function> attached,
      v8::MaybeLocal<v8::Function> detached,
      v8::MaybeLocal<v8::Function> attributeChanged);

  void created(Element*) override;
  void attached(Element*) override;
  void detached(Element*) override;
  void attributeChanged(Element*,
                        const AtomicString& name,
                        const AtomicString& oldValue,
                        const AtomicString& newValue) override;

  void call(const ScopedPersistent<v8::Function>& weakCallback, Element*);

  V8PerContextData* creationContextData();

  RefPtr<ScriptState> m_scriptState;
  ScopedPersistent<v8::Object> m_prototype;
  ScopedPersistent<v8::Function> m_created;
  ScopedPersistent<v8::Function> m_attached;
  ScopedPersistent<v8::Function> m_detached;
  ScopedPersistent<v8::Function> m_attributeChanged;
};

}  // namespace blink

#endif  // V8V0CustomElementLifecycleCallbacks_h
