// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/paint/PaintInvalidationCapableScrollableArea.h"

#include "core/frame/Settings.h"
#include "core/frame/UseCounter.h"
#include "core/html/HTMLFrameOwnerElement.h"
#include "core/layout/LayoutBox.h"
#include "core/layout/LayoutScrollbar.h"
#include "core/layout/LayoutScrollbarPart.h"
#include "core/layout/PaintInvalidationState.h"
#include "core/paint/ObjectPaintInvalidator.h"
#include "core/paint/PaintInvalidator.h"
#include "core/paint/PaintLayer.h"
#include "platform/graphics/GraphicsLayer.h"

namespace blink {

void PaintInvalidationCapableScrollableArea::willRemoveScrollbar(
    Scrollbar& scrollbar,
    ScrollbarOrientation orientation) {
  if (!scrollbar.isCustomScrollbar() &&
      !(orientation == HorizontalScrollbar ? layerForHorizontalScrollbar()
                                           : layerForVerticalScrollbar()))
    ObjectPaintInvalidator(*layoutBox())
        .slowSetPaintingLayerNeedsRepaintAndInvalidateDisplayItemClient(
            scrollbar, PaintInvalidationScroll);

  ScrollableArea::willRemoveScrollbar(scrollbar, orientation);
}

static LayoutRect scrollControlVisualRect(
    const IntRect& scrollControlRect,
    const LayoutBox& box,
    const PaintInvalidatorContext& context) {
  LayoutRect visualRect(scrollControlRect);
  // No need to apply any paint offset. Scroll controls paint in a different
  // transform space than their contained box (the scrollbarPaintOffset
  // transform node).
  if (!visualRect.isEmpty() &&
      !RuntimeEnabledFeatures::slimmingPaintV2Enabled()) {
    // PaintInvalidatorContext::mapLocalRectToPaintInvalidationBacking() treats
    // the rect as in flipped block direction, but scrollbar controls don't
    // flip for block direction, so flip here to undo the flip in the function.
    box.flipForWritingMode(visualRect);
    context.mapLocalRectToVisualRectInBacking(box, visualRect);
  }
  return visualRect;
}

// Returns true if the scroll control is invalidated.
static bool invalidatePaintOfScrollControlIfNeeded(
    const LayoutRect& newVisualRect,
    const LayoutRect& previousVisualRect,
    bool needsPaintInvalidation,
    LayoutBox& box,
    const LayoutBoxModelObject& paintInvalidationContainer) {
  bool shouldInvalidateNewRect = needsPaintInvalidation;
  if (newVisualRect != previousVisualRect) {
    ObjectPaintInvalidator(box).invalidatePaintUsingContainer(
        paintInvalidationContainer, previousVisualRect,
        PaintInvalidationScroll);
    shouldInvalidateNewRect = true;
  }
  if (shouldInvalidateNewRect) {
    ObjectPaintInvalidator(box).invalidatePaintUsingContainer(
        paintInvalidationContainer, newVisualRect, PaintInvalidationScroll);
    return true;
  }
  return false;
}

static LayoutRect invalidatePaintOfScrollbarIfNeeded(
    Scrollbar* scrollbar,
    GraphicsLayer* graphicsLayer,
    bool& previouslyWasOverlay,
    const LayoutRect& previousVisualRect,
    bool needsPaintInvalidationArg,
    LayoutBox& box,
    const PaintInvalidatorContext& context) {
  bool isOverlay = scrollbar && scrollbar->isOverlayScrollbar();

  // Calculate visual rect of the scrollbar, except overlay composited
  // scrollbars because we invalidate the graphics layer only.
  LayoutRect newVisualRect;
  if (scrollbar && !(graphicsLayer && isOverlay)) {
    newVisualRect =
        scrollControlVisualRect(scrollbar->frameRect(), box, context);
  }

  bool needsPaintInvalidation = needsPaintInvalidationArg;
  if (needsPaintInvalidation && graphicsLayer) {
    // If the scrollbar needs paint invalidation but didn't change location/size
    // or the scrollbar is an overlay scrollbar (visual rect is empty),
    // invalidating the graphics layer is enough (which has been done in
    // ScrollableArea::setScrollbarNeedsPaintInvalidation()).
    // Otherwise invalidatePaintOfScrollControlIfNeeded() below will invalidate
    // the old and new location of the scrollbar on the box's paint invalidation
    // container to ensure newly expanded/shrunk areas of the box to be
    // invalidated.
    needsPaintInvalidation = false;
    DCHECK(!graphicsLayer->drawsContent() ||
           graphicsLayer->getPaintController().cacheIsEmpty());
  }

  // Invalidate the box's display item client if the box's padding box size is
  // affected by change of the non-overlay scrollbar width. We detect change of
  // visual rect size instead of change of scrollbar width change, which may
  // have some false-positives (e.g. the scrollbar changed length but not width)
  // but won't invalidate more than expected because in the false-positive case
  // the box must have changed size and have been invalidated.
  const LayoutBoxModelObject& paintInvalidationContainer =
      *context.paintInvalidationContainer;
  LayoutSize newScrollbarUsedSpaceInBox;
  if (!isOverlay)
    newScrollbarUsedSpaceInBox = newVisualRect.size();
  LayoutSize previousScrollbarUsedSpaceInBox;
  if (!previouslyWasOverlay)
    previousScrollbarUsedSpaceInBox = previousVisualRect.size();
  if (newScrollbarUsedSpaceInBox != previousScrollbarUsedSpaceInBox) {
    context.paintingLayer->setNeedsRepaint();
    ObjectPaintInvalidator(box).invalidateDisplayItemClient(
        box, PaintInvalidationScroll);
  }

  bool invalidated = invalidatePaintOfScrollControlIfNeeded(
      newVisualRect, previousVisualRect, needsPaintInvalidation, box,
      paintInvalidationContainer);

  previouslyWasOverlay = isOverlay;

  if (!invalidated || !scrollbar || graphicsLayer)
    return newVisualRect;

  context.paintingLayer->setNeedsRepaint();
  ObjectPaintInvalidator(box).invalidateDisplayItemClient(
      *scrollbar, PaintInvalidationScroll);
  if (scrollbar->isCustomScrollbar()) {
    toLayoutScrollbar(scrollbar)
        ->invalidateDisplayItemClientsOfScrollbarParts();
  }

  return newVisualRect;
}

void PaintInvalidationCapableScrollableArea::
    invalidatePaintOfScrollControlsIfNeeded(
        const PaintInvalidatorContext& context) {
  LayoutBox& box = *layoutBox();
  setHorizontalScrollbarVisualRect(invalidatePaintOfScrollbarIfNeeded(
      horizontalScrollbar(), layerForHorizontalScrollbar(),
      m_horizontalScrollbarPreviouslyWasOverlay,
      m_horizontalScrollbarVisualRect,
      horizontalScrollbarNeedsPaintInvalidation(), box, context));
  setVerticalScrollbarVisualRect(invalidatePaintOfScrollbarIfNeeded(
      verticalScrollbar(), layerForVerticalScrollbar(),
      m_verticalScrollbarPreviouslyWasOverlay, m_verticalScrollbarVisualRect,
      verticalScrollbarNeedsPaintInvalidation(), box, context));

  LayoutRect scrollCornerAndResizerVisualRect =
      scrollControlVisualRect(scrollCornerAndResizerRect(), box, context);
  const LayoutBoxModelObject& paintInvalidationContainer =
      *context.paintInvalidationContainer;
  if (invalidatePaintOfScrollControlIfNeeded(
          scrollCornerAndResizerVisualRect, m_scrollCornerAndResizerVisualRect,
          scrollCornerNeedsPaintInvalidation(), box,
          paintInvalidationContainer)) {
    setScrollCornerAndResizerVisualRect(scrollCornerAndResizerVisualRect);
    if (LayoutScrollbarPart* scrollCorner = this->scrollCorner()) {
      ObjectPaintInvalidator(*scrollCorner)
          .invalidateDisplayItemClientsIncludingNonCompositingDescendants(
              PaintInvalidationScroll);
    }
    if (LayoutScrollbarPart* resizer = this->resizer()) {
      ObjectPaintInvalidator(*resizer)
          .invalidateDisplayItemClientsIncludingNonCompositingDescendants(
              PaintInvalidationScroll);
    }
  }

  clearNeedsPaintInvalidationForScrollControls();
}

void PaintInvalidationCapableScrollableArea::
    invalidatePaintOfScrollControlsIfNeeded(
        const PaintInvalidationState& paintInvalidationState) {
  invalidatePaintOfScrollControlsIfNeeded(
      PaintInvalidatorContextAdapter(paintInvalidationState));
}

void PaintInvalidationCapableScrollableArea::clearPreviousVisualRects() {
  setHorizontalScrollbarVisualRect(LayoutRect());
  setVerticalScrollbarVisualRect(LayoutRect());
  setScrollCornerAndResizerVisualRect(LayoutRect());
}

void PaintInvalidationCapableScrollableArea::setHorizontalScrollbarVisualRect(
    const LayoutRect& rect) {
  m_horizontalScrollbarVisualRect = rect;
  if (Scrollbar* scrollbar = horizontalScrollbar())
    scrollbar->setVisualRect(rect);
}

void PaintInvalidationCapableScrollableArea::setVerticalScrollbarVisualRect(
    const LayoutRect& rect) {
  m_verticalScrollbarVisualRect = rect;
  if (Scrollbar* scrollbar = verticalScrollbar())
    scrollbar->setVisualRect(rect);
}

void PaintInvalidationCapableScrollableArea::
    setScrollCornerAndResizerVisualRect(const LayoutRect& rect) {
  m_scrollCornerAndResizerVisualRect = rect;
  if (LayoutScrollbarPart* scrollCorner = this->scrollCorner())
    scrollCorner->setVisualRect(rect);
  if (LayoutScrollbarPart* resizer = this->resizer())
    resizer->setVisualRect(rect);
}

void PaintInvalidationCapableScrollableArea::
    scrollControlWasSetNeedsPaintInvalidation() {
  layoutBox()->setMayNeedPaintInvalidation();
}

void PaintInvalidationCapableScrollableArea::didScrollWithScrollbar(
    ScrollbarPart part,
    ScrollbarOrientation orientation) {
  UseCounter::Feature scrollbarUseUMA;
  switch (part) {
    case BackButtonStartPart:
    case ForwardButtonStartPart:
    case BackButtonEndPart:
    case ForwardButtonEndPart:
      scrollbarUseUMA =
          (orientation == VerticalScrollbar
               ? UseCounter::ScrollbarUseVerticalScrollbarButton
               : UseCounter::ScrollbarUseHorizontalScrollbarButton);
      break;
    case ThumbPart:
      scrollbarUseUMA =
          (orientation == VerticalScrollbar
               ? UseCounter::ScrollbarUseVerticalScrollbarThumb
               : UseCounter::ScrollbarUseHorizontalScrollbarThumb);
      break;
    case BackTrackPart:
    case ForwardTrackPart:
      scrollbarUseUMA =
          (orientation == VerticalScrollbar
               ? UseCounter::ScrollbarUseVerticalScrollbarTrack
               : UseCounter::ScrollbarUseHorizontalScrollbarTrack);
      break;
    default:
      return;
  }

  UseCounter::count(layoutBox()->document(), scrollbarUseUMA);
}

}  // namespace blink
