// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/blink/web_scrollbar_layer_impl.h"

#include <utility>

#include "base/memory/ptr_util.h"
#include "cc/blink/scrollbar_impl.h"
#include "cc/blink/web_layer_impl.h"
#include "cc/layers/layer.h"
#include "cc/layers/painted_overlay_scrollbar_layer.h"
#include "cc/layers/painted_scrollbar_layer.h"
#include "cc/layers/scrollbar_layer_interface.h"
#include "cc/layers/solid_color_scrollbar_layer.h"

using cc::PaintedOverlayScrollbarLayer;
using cc::PaintedScrollbarLayer;
using cc::SolidColorScrollbarLayer;

namespace {

cc::ScrollbarOrientation ConvertOrientation(
    blink::WebScrollbar::Orientation orientation) {
  return orientation == blink::WebScrollbar::Horizontal ? cc::HORIZONTAL
                                                        : cc::VERTICAL;
}

}  // namespace

namespace cc_blink {

WebScrollbarLayerImpl::WebScrollbarLayerImpl(
    std::unique_ptr<blink::WebScrollbar> scrollbar,
    blink::WebScrollbarThemePainter painter,
    std::unique_ptr<blink::WebScrollbarThemeGeometry> geometry)
    : layer_(new WebLayerImpl(PaintedScrollbarLayer::Create(
          base::MakeUnique<ScrollbarImpl>(std::move(scrollbar),
                                          painter,
                                          std::move(geometry)),
          cc::Layer::INVALID_ID))) {}

WebScrollbarLayerImpl::WebScrollbarLayerImpl(
    std::unique_ptr<blink::WebScrollbar> scrollbar,
    blink::WebScrollbarThemePainter painter,
    std::unique_ptr<blink::WebScrollbarThemeGeometry> geometry,
    bool)
    : layer_(new WebLayerImpl(PaintedOverlayScrollbarLayer::Create(
          base::MakeUnique<ScrollbarImpl>(std::move(scrollbar),
                                          painter,
                                          std::move(geometry)),
          cc::Layer::INVALID_ID))) {}

WebScrollbarLayerImpl::WebScrollbarLayerImpl(
    blink::WebScrollbar::Orientation orientation,
    int thumb_thickness,
    int track_start,
    bool is_left_side_vertical_scrollbar)
    : layer_(new WebLayerImpl(
          SolidColorScrollbarLayer::Create(ConvertOrientation(orientation),
                                           thumb_thickness,
                                           track_start,
                                           is_left_side_vertical_scrollbar,
                                           cc::Layer::INVALID_ID))) {}

WebScrollbarLayerImpl::~WebScrollbarLayerImpl() {
}

blink::WebLayer* WebScrollbarLayerImpl::layer() {
  return layer_.get();
}

void WebScrollbarLayerImpl::setScrollLayer(blink::WebLayer* layer) {
  cc::Layer* scroll_layer =
      layer ? static_cast<WebLayerImpl*>(layer)->layer() : 0;
  layer_->layer()->ToScrollbarLayer()->SetScrollLayer(
      scroll_layer ? scroll_layer->id() : cc::Layer::INVALID_ID);
}

}  // namespace cc_blink
