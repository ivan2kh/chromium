/*
 * Copyright (C) 2012 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "public/platform/WebScrollbarThemePainter.h"

#include "platform/graphics/GraphicsContext.h"
#include "platform/graphics/paint/PaintCanvas.h"
#include "platform/graphics/paint/PaintRecordBuilder.h"
#include "platform/scroll/Scrollbar.h"
#include "platform/scroll/ScrollbarTheme.h"
#include "public/platform/WebRect.h"

namespace blink {

void WebScrollbarThemePainter::assign(const WebScrollbarThemePainter& painter) {
  // This is a pointer to a static object, so no ownership transferral.
  m_theme = painter.m_theme;
  m_scrollbar = painter.m_scrollbar;
  m_deviceScaleFactor = painter.m_deviceScaleFactor;
}

void WebScrollbarThemePainter::reset() {
  m_scrollbar = nullptr;
}

void WebScrollbarThemePainter::paintScrollbarBackground(WebCanvas* canvas,
                                                        const WebRect& rect) {
  SkRect clip = SkRect::MakeXYWH(rect.x, rect.y, rect.width, rect.height);
  canvas->clipRect(clip);

  IntRect intRect(rect);
  PaintRecordBuilder builder(intRect);
  builder.context().setDeviceScaleFactor(m_deviceScaleFactor);
  m_theme->paintScrollbarBackground(builder.context(), *m_scrollbar);
  canvas->drawPicture(builder.endRecording());
}

void WebScrollbarThemePainter::paintTrackBackground(WebCanvas* canvas,
                                                    const WebRect& rect) {
  IntRect intRect(rect);
  PaintRecordBuilder builder(intRect);
  builder.context().setDeviceScaleFactor(m_deviceScaleFactor);
  m_theme->paintTrackBackground(builder.context(), *m_scrollbar, intRect);
  canvas->drawPicture(builder.endRecording());
  if (!m_theme->shouldRepaintAllPartsOnInvalidation())
    m_scrollbar->clearTrackNeedsRepaint();
}

void WebScrollbarThemePainter::paintBackTrackPart(WebCanvas* canvas,
                                                  const WebRect& rect) {
  IntRect intRect(rect);
  PaintRecordBuilder builder(intRect);
  builder.context().setDeviceScaleFactor(m_deviceScaleFactor);
  m_theme->paintTrackPiece(builder.context(), *m_scrollbar, intRect,
                           BackTrackPart);
  canvas->drawPicture(builder.endRecording());
}

void WebScrollbarThemePainter::paintForwardTrackPart(WebCanvas* canvas,
                                                     const WebRect& rect) {
  IntRect intRect(rect);
  PaintRecordBuilder builder(intRect);
  builder.context().setDeviceScaleFactor(m_deviceScaleFactor);
  m_theme->paintTrackPiece(builder.context(), *m_scrollbar, intRect,
                           ForwardTrackPart);
  canvas->drawPicture(builder.endRecording());
}

void WebScrollbarThemePainter::paintBackButtonStart(WebCanvas* canvas,
                                                    const WebRect& rect) {
  IntRect intRect(rect);
  PaintRecordBuilder builder(intRect);
  builder.context().setDeviceScaleFactor(m_deviceScaleFactor);
  m_theme->paintButton(builder.context(), *m_scrollbar, intRect,
                       BackButtonStartPart);
  canvas->drawPicture(builder.endRecording());
}

void WebScrollbarThemePainter::paintBackButtonEnd(WebCanvas* canvas,
                                                  const WebRect& rect) {
  IntRect intRect(rect);
  PaintRecordBuilder builder(intRect);
  builder.context().setDeviceScaleFactor(m_deviceScaleFactor);
  m_theme->paintButton(builder.context(), *m_scrollbar, intRect,
                       BackButtonEndPart);
  canvas->drawPicture(builder.endRecording());
}

void WebScrollbarThemePainter::paintForwardButtonStart(WebCanvas* canvas,
                                                       const WebRect& rect) {
  IntRect intRect(rect);
  PaintRecordBuilder builder(intRect);
  builder.context().setDeviceScaleFactor(m_deviceScaleFactor);
  m_theme->paintButton(builder.context(), *m_scrollbar, intRect,
                       ForwardButtonStartPart);
  canvas->drawPicture(builder.endRecording());
}

void WebScrollbarThemePainter::paintForwardButtonEnd(WebCanvas* canvas,
                                                     const WebRect& rect) {
  IntRect intRect(rect);
  PaintRecordBuilder builder(intRect);
  builder.context().setDeviceScaleFactor(m_deviceScaleFactor);
  m_theme->paintButton(builder.context(), *m_scrollbar, intRect,
                       ForwardButtonEndPart);
  canvas->drawPicture(builder.endRecording());
}

void WebScrollbarThemePainter::paintTickmarks(WebCanvas* canvas,
                                              const WebRect& rect) {
  IntRect intRect(rect);
  PaintRecordBuilder builder(intRect);
  builder.context().setDeviceScaleFactor(m_deviceScaleFactor);
  m_theme->paintTickmarks(builder.context(), *m_scrollbar, intRect);
  canvas->drawPicture(builder.endRecording());
}

void WebScrollbarThemePainter::paintThumb(WebCanvas* canvas,
                                          const WebRect& rect) {
  IntRect intRect(rect);
  PaintRecordBuilder builder(intRect);
  builder.context().setDeviceScaleFactor(m_deviceScaleFactor);
  m_theme->paintThumb(builder.context(), *m_scrollbar, intRect);
  canvas->drawPicture(builder.endRecording());
  if (!m_theme->shouldRepaintAllPartsOnInvalidation())
    m_scrollbar->clearThumbNeedsRepaint();
}

WebScrollbarThemePainter::WebScrollbarThemePainter(ScrollbarTheme& theme,
                                                   Scrollbar& scrollbar,
                                                   float deviceScaleFactor)
    : m_theme(&theme),
      m_scrollbar(&scrollbar),
      m_deviceScaleFactor(deviceScaleFactor) {}

float WebScrollbarThemePainter::thumbOpacity() const {
  return m_theme->thumbOpacity(*m_scrollbar);
}

bool WebScrollbarThemePainter::trackNeedsRepaint() const {
  return m_scrollbar->trackNeedsRepaint();
}

bool WebScrollbarThemePainter::thumbNeedsRepaint() const {
  return m_scrollbar->thumbNeedsRepaint();
}

bool WebScrollbarThemePainter::usesNinePatchThumbResource() const {
  return m_theme->usesNinePatchThumbResource();
}

}  // namespace blink
