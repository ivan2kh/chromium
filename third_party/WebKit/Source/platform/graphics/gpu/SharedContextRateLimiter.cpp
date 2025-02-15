// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "platform/graphics/gpu/SharedContextRateLimiter.h"

#include "gpu/GLES2/gl2extchromium.h"
#include "platform/graphics/gpu/Extensions3DUtil.h"
#include "public/platform/Platform.h"
#include "public/platform/WebGraphicsContext3DProvider.h"
#include "third_party/khronos/GLES2/gl2.h"
#include "wtf/PtrUtil.h"
#include <memory>

namespace blink {

std::unique_ptr<SharedContextRateLimiter> SharedContextRateLimiter::create(
    unsigned maxPendingTicks) {
  return WTF::wrapUnique(new SharedContextRateLimiter(maxPendingTicks));
}

SharedContextRateLimiter::SharedContextRateLimiter(unsigned maxPendingTicks)
    : m_maxPendingTicks(maxPendingTicks), m_canUseSyncQueries(false) {
  m_contextProvider = WTF::wrapUnique(
      Platform::current()->createSharedOffscreenGraphicsContext3DProvider());
  if (!m_contextProvider)
    return;

  gpu::gles2::GLES2Interface* gl = m_contextProvider->contextGL();
  if (gl && gl->GetGraphicsResetStatusKHR() == GL_NO_ERROR) {
    std::unique_ptr<Extensions3DUtil> extensionsUtil =
        Extensions3DUtil::create(gl);
    // TODO(junov): when the GLES 3.0 command buffer is ready, we could use
    // fenceSync instead.
    m_canUseSyncQueries =
        extensionsUtil->supportsExtension("GL_CHROMIUM_sync_query");
  }
}

void SharedContextRateLimiter::tick() {
  if (!m_contextProvider)
    return;

  gpu::gles2::GLES2Interface* gl = m_contextProvider->contextGL();
  if (!gl || gl->GetGraphicsResetStatusKHR() != GL_NO_ERROR)
    return;

  m_queries.push_back(0);
  if (m_canUseSyncQueries)
    gl->GenQueriesEXT(1, &m_queries.back());
  if (m_canUseSyncQueries) {
    gl->BeginQueryEXT(GL_COMMANDS_COMPLETED_CHROMIUM, m_queries.back());
    gl->EndQueryEXT(GL_COMMANDS_COMPLETED_CHROMIUM);
  }
  if (m_queries.size() > m_maxPendingTicks) {
    if (m_canUseSyncQueries) {
      GLuint result;
      gl->GetQueryObjectuivEXT(m_queries.front(), GL_QUERY_RESULT_EXT, &result);
      gl->DeleteQueriesEXT(1, &m_queries.front());
      m_queries.pop_front();
    } else {
      gl->Finish();
      reset();
    }
  }
}

void SharedContextRateLimiter::reset() {
  if (!m_contextProvider)
    return;

  gpu::gles2::GLES2Interface* gl = m_contextProvider->contextGL();
  if (gl && gl->GetGraphicsResetStatusKHR() == GL_NO_ERROR) {
    while (m_queries.size() > 0) {
      gl->DeleteQueriesEXT(1, &m_queries.front());
      m_queries.pop_front();
    }
  } else {
    m_queries.clear();
  }
}

}  // namespace blink
