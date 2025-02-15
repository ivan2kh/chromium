// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef InspectorRenderingAgent_h
#define InspectorRenderingAgent_h

#include "core/inspector/InspectorBaseAgent.h"
#include "core/inspector/protocol/Rendering.h"

namespace blink {

class InspectorOverlay;
class WebLocalFrameImpl;
class WebViewImpl;

class InspectorRenderingAgent final
    : public InspectorBaseAgent<protocol::Rendering::Metainfo> {
  WTF_MAKE_NONCOPYABLE(InspectorRenderingAgent);

 public:
  static InspectorRenderingAgent* create(WebLocalFrameImpl*, InspectorOverlay*);

  // protocol::Dispatcher::PageCommandHandler implementation.
  protocol::Response setShowPaintRects(bool) override;
  protocol::Response setShowDebugBorders(bool) override;
  protocol::Response setShowFPSCounter(bool) override;
  protocol::Response setShowScrollBottleneckRects(bool) override;
  protocol::Response setShowViewportSizeOnResize(bool) override;

  // InspectorBaseAgent overrides.
  protocol::Response disable() override;
  void restore() override;

  DECLARE_VIRTUAL_TRACE();

 private:
  InspectorRenderingAgent(WebLocalFrameImpl*, InspectorOverlay*);
  protocol::Response compositingEnabled();
  WebViewImpl* webViewImpl();

  Member<WebLocalFrameImpl> m_webLocalFrameImpl;
  Member<InspectorOverlay> m_overlay;
};

}  // namespace blink

#endif  // !defined(InspectorRenderingAgent_h)
