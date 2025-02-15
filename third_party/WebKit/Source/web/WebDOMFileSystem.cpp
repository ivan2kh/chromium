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

#include "public/web/WebDOMFileSystem.h"

#include "bindings/core/v8/WrapperTypeInfo.h"
#include "bindings/modules/v8/V8DOMFileSystem.h"
#include "bindings/modules/v8/V8DirectoryEntry.h"
#include "bindings/modules/v8/V8FileEntry.h"
#include "core/dom/Document.h"
#include "modules/filesystem/DOMFileSystem.h"
#include "modules/filesystem/DirectoryEntry.h"
#include "modules/filesystem/FileEntry.h"
#include "v8/include/v8.h"
#include "web/WebLocalFrameImpl.h"

namespace blink {

WebDOMFileSystem WebDOMFileSystem::fromV8Value(v8::Local<v8::Value> value) {
  if (!V8DOMFileSystem::hasInstance(value, v8::Isolate::GetCurrent()))
    return WebDOMFileSystem();
  v8::Local<v8::Object> object = v8::Local<v8::Object>::Cast(value);
  DOMFileSystem* domFileSystem = V8DOMFileSystem::toImpl(object);
  DCHECK(domFileSystem);
  return WebDOMFileSystem(domFileSystem);
}

WebURL WebDOMFileSystem::createFileSystemURL(v8::Local<v8::Value> value) {
  const Entry* const entry =
      V8Entry::toImplWithTypeCheck(v8::Isolate::GetCurrent(), value);
  if (entry)
    return entry->filesystem()->createFileSystemURL(entry);
  return WebURL();
}

WebDOMFileSystem WebDOMFileSystem::create(WebLocalFrame* frame,
                                          WebFileSystemType type,
                                          const WebString& name,
                                          const WebURL& rootURL,
                                          SerializableType serializableType) {
  DCHECK(frame);
  DCHECK(toWebLocalFrameImpl(frame)->frame());
  DOMFileSystem* domFileSystem =
      DOMFileSystem::create(toWebLocalFrameImpl(frame)->frame()->document(),
                            name, static_cast<FileSystemType>(type), rootURL);
  if (serializableType == SerializableTypeSerializable)
    domFileSystem->makeClonable();
  return WebDOMFileSystem(domFileSystem);
}

void WebDOMFileSystem::reset() {
  m_private.reset();
}

void WebDOMFileSystem::assign(const WebDOMFileSystem& other) {
  m_private = other.m_private;
}

WebString WebDOMFileSystem::name() const {
  DCHECK(m_private.get());
  return m_private->name();
}

WebFileSystem::Type WebDOMFileSystem::type() const {
  DCHECK(m_private.get());
  switch (m_private->type()) {
    case FileSystemTypeTemporary:
      return WebFileSystem::TypeTemporary;
    case FileSystemTypePersistent:
      return WebFileSystem::TypePersistent;
    case FileSystemTypeIsolated:
      return WebFileSystem::TypeIsolated;
    case FileSystemTypeExternal:
      return WebFileSystem::TypeExternal;
    default:
      NOTREACHED();
      return WebFileSystem::TypeTemporary;
  }
}

WebURL WebDOMFileSystem::rootURL() const {
  DCHECK(m_private.get());
  return m_private->rootURL();
}

v8::Local<v8::Value> WebDOMFileSystem::toV8Value(
    v8::Local<v8::Object> creationContext,
    v8::Isolate* isolate) {
  // We no longer use |creationContext| because it's often misused and points
  // to a context faked by user script.
  DCHECK(creationContext->CreationContext() == isolate->GetCurrentContext());
  if (!m_private.get())
    return v8::Local<v8::Value>();
  return ToV8(m_private.get(), isolate->GetCurrentContext()->Global(), isolate);
}

v8::Local<v8::Value> WebDOMFileSystem::createV8Entry(
    const WebString& path,
    EntryType entryType,
    v8::Local<v8::Object> creationContext,
    v8::Isolate* isolate) {
  // We no longer use |creationContext| because it's often misused and points
  // to a context faked by user script.
  DCHECK(creationContext->CreationContext() == isolate->GetCurrentContext());
  if (!m_private.get())
    return v8::Local<v8::Value>();
  if (entryType == EntryTypeDirectory)
    return ToV8(DirectoryEntry::create(m_private.get(), path),
                isolate->GetCurrentContext()->Global(), isolate);
  DCHECK_EQ(entryType, EntryTypeFile);
  return ToV8(FileEntry::create(m_private.get(), path),
              isolate->GetCurrentContext()->Global(), isolate);
}

WebDOMFileSystem::WebDOMFileSystem(DOMFileSystem* domFileSystem)
    : m_private(domFileSystem) {}

WebDOMFileSystem& WebDOMFileSystem::operator=(DOMFileSystem* domFileSystem) {
  m_private = domFileSystem;
  return *this;
}

}  // namespace blink
