/*
 Copyright (C) 1998 Lars Knoll (knoll@mpi-hd.mpg.de)
 Copyright (C) 2001 Dirk Mueller <mueller@kde.org>
 Copyright (C) 2004, 2005, 2006, 2007, 2008 Apple Inc. All rights reserved.
 Copyright (C) 2011 Google Inc. All rights reserved.

 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Library General Public
 License as published by the Free Software Foundation; either
 version 2 of the License, or (at your option) any later version.

 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Library General Public License for more details.

 You should have received a copy of the GNU Library General Public License
 along with this library; see the file COPYING.LIB.  If not, write to
 the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 Boston, MA 02110-1301, USA.

 This class provides all functionality needed for loading images, style sheets
 and html pages from the web. It has a memory cache for these objects.
 */

#ifndef StyleSheetResourceClient_h
#define StyleSheetResourceClient_h

#include "core/CoreExport.h"
#include "platform/loader/fetch/ResourceClient.h"
#include "platform/weborigin/KURL.h"
#include "wtf/Forward.h"

namespace blink {
class CSSStyleSheetResource;

class CORE_EXPORT StyleSheetResourceClient : public ResourceClient {
 public:
  static bool isExpectedType(ResourceClient* client) {
    return client->getResourceClientType() == StyleSheetType;
  }
  ResourceClientType getResourceClientType() const final {
    return StyleSheetType;
  }
  virtual void setCSSStyleSheet(const String& /* href */,
                                const KURL& /* baseURL */,
                                const String& /* charset */,
                                const CSSStyleSheetResource*) {}
  virtual void setXSLStyleSheet(const String& /* href */,
                                const KURL& /* baseURL */,
                                const String& /* sheet */) {}

  // This gets called on the very first appendData call for the
  // CSSStyleSheetResource. Note this is not called for StyleSheetResources
  // other than CSSStyleSheetResources.
  virtual void didAppendFirstData(const CSSStyleSheetResource*) {}

  DEFINE_INLINE_TRACE() { ResourceClient::trace(visitor); }
};

}  // namespace blink

#endif  // StyleSheetResourceClient_h
