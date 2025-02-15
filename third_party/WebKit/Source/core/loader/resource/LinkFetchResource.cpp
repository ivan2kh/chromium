// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/loader/resource/LinkFetchResource.h"

#include "platform/loader/fetch/FetchRequest.h"
#include "platform/loader/fetch/ResourceFetcher.h"

namespace blink {

Resource* LinkFetchResource::fetch(Resource::Type type,
                                   FetchRequest& request,
                                   ResourceFetcher* fetcher) {
  DCHECK_EQ(type, LinkPrefetch);
  DCHECK_EQ(request.resourceRequest().frameType(),
            WebURLRequest::FrameTypeNone);
  request.setRequestContext(fetcher->determineRequestContext(type));
  return fetcher->requestResource(request, LinkResourceFactory(type));
}

LinkFetchResource::LinkFetchResource(const ResourceRequest& request,
                                     Type type,
                                     const ResourceLoaderOptions& options)
    : Resource(request, type, options) {}

LinkFetchResource::~LinkFetchResource() {}

}  // namespace blink
