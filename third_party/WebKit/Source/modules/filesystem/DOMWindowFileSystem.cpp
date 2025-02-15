/*
 * Copyright (C) 2012, Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 */

#include "modules/filesystem/DOMWindowFileSystem.h"

#include "core/dom/Document.h"
#include "core/fileapi/FileError.h"
#include "core/frame/LocalDOMWindow.h"
#include "core/frame/UseCounter.h"
#include "modules/filesystem/DOMFileSystem.h"
#include "modules/filesystem/EntryCallback.h"
#include "modules/filesystem/ErrorCallback.h"
#include "modules/filesystem/FileSystemCallback.h"
#include "modules/filesystem/FileSystemCallbacks.h"
#include "modules/filesystem/LocalFileSystem.h"
#include "platform/FileSystemType.h"
#include "platform/weborigin/SchemeRegistry.h"
#include "platform/weborigin/SecurityOrigin.h"

namespace blink {

void DOMWindowFileSystem::webkitRequestFileSystem(
    LocalDOMWindow& window,
    int type,
    long long size,
    FileSystemCallback* successCallback,
    ErrorCallback* errorCallback) {
  if (!window.isCurrentlyDisplayedInFrame())
    return;

  Document* document = window.document();
  if (!document)
    return;

  if (SchemeRegistry::schemeShouldBypassContentSecurityPolicy(
          document->getSecurityOrigin()->protocol()))
    UseCounter::count(document, UseCounter::RequestFileSystemNonWebbyOrigin);

  if (!document->getSecurityOrigin()->canAccessFileSystem()) {
    DOMFileSystem::reportError(document,
                               ScriptErrorCallback::wrap(errorCallback),
                               FileError::kSecurityErr);
    return;
  }

  FileSystemType fileSystemType = static_cast<FileSystemType>(type);
  if (!DOMFileSystemBase::isValidType(fileSystemType)) {
    DOMFileSystem::reportError(document,
                               ScriptErrorCallback::wrap(errorCallback),
                               FileError::kInvalidModificationErr);
    return;
  }

  LocalFileSystem::from(*document)->requestFileSystem(
      document, fileSystemType, size,
      FileSystemCallbacks::create(successCallback,
                                  ScriptErrorCallback::wrap(errorCallback),
                                  document, fileSystemType));
}

void DOMWindowFileSystem::webkitResolveLocalFileSystemURL(
    LocalDOMWindow& window,
    const String& url,
    EntryCallback* successCallback,
    ErrorCallback* errorCallback) {
  if (!window.isCurrentlyDisplayedInFrame())
    return;

  Document* document = window.document();
  if (!document)
    return;

  SecurityOrigin* securityOrigin = document->getSecurityOrigin();
  KURL completedURL = document->completeURL(url);
  if (!securityOrigin->canAccessFileSystem() ||
      !securityOrigin->canRequest(completedURL)) {
    DOMFileSystem::reportError(document,
                               ScriptErrorCallback::wrap(errorCallback),
                               FileError::kSecurityErr);
    return;
  }

  if (!completedURL.isValid()) {
    DOMFileSystem::reportError(document,
                               ScriptErrorCallback::wrap(errorCallback),
                               FileError::kEncodingErr);
    return;
  }

  LocalFileSystem::from(*document)->resolveURL(
      document, completedURL,
      ResolveURICallbacks::create(
          successCallback, ScriptErrorCallback::wrap(errorCallback), document));
}

static_assert(
    static_cast<int>(DOMWindowFileSystem::kTemporary) ==
        static_cast<int>(FileSystemTypeTemporary),
    "DOMWindowFileSystem::kTemporary should match FileSystemTypeTemporary");
static_assert(
    static_cast<int>(DOMWindowFileSystem::kPersistent) ==
        static_cast<int>(FileSystemTypePersistent),
    "DOMWindowFileSystem::kPersistent should match FileSystemTypePersistent");

}  // namespace blink
