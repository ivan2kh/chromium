// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef TextPainter_h
#define TextPainter_h

#include "core/CoreExport.h"
#include "core/style/ComputedStyleConstants.h"
#include "platform/geometry/FloatPoint.h"
#include "platform/geometry/FloatRect.h"
#include "platform/geometry/LayoutRect.h"
#include "platform/graphics/Color.h"
#include "platform/transforms/AffineTransform.h"
#include "wtf/Allocator.h"
#include "wtf/text/AtomicString.h"

namespace blink {

class ComputedStyle;
class Font;
class GraphicsContext;
class GraphicsContextStateSaver;
class LayoutTextCombine;
class LineLayoutItem;
struct PaintInfo;
class ShadowList;
class TextRun;
struct TextRunPaintInfo;

class CORE_EXPORT TextPainter {
  STACK_ALLOCATED();

 public:
  struct Style;

  TextPainter(GraphicsContext&,
              const Font&,
              const TextRun&,
              const LayoutPoint& textOrigin,
              const LayoutRect& textBounds,
              bool horizontal);
  ~TextPainter();

  void setEmphasisMark(const AtomicString&, TextEmphasisPosition);
  void setCombinedText(LayoutTextCombine* combinedText) {
    m_combinedText = combinedText;
  }
  void setEllipsisOffset(int offset) { m_ellipsisOffset = offset; }

  static void updateGraphicsContext(GraphicsContext&,
                                    const Style&,
                                    bool horizontal,
                                    GraphicsContextStateSaver&);

  void clipDecorationsStripe(float upper, float stripeWidth, float dilation);
  void paint(unsigned startOffset,
             unsigned endOffset,
             unsigned length,
             const Style&);

  struct Style {
    STACK_ALLOCATED();
    Color currentColor;
    Color fillColor;
    Color strokeColor;
    Color emphasisMarkColor;
    float strokeWidth;
    const ShadowList* shadow;

    bool operator==(const Style& other) {
      return currentColor == other.currentColor &&
             fillColor == other.fillColor && strokeColor == other.strokeColor &&
             emphasisMarkColor == other.emphasisMarkColor &&
             strokeWidth == other.strokeWidth && shadow == other.shadow;
    }
    bool operator!=(const Style& other) { return !(*this == other); }
  };
  static Style textPaintingStyle(LineLayoutItem,
                                 const ComputedStyle&,
                                 const PaintInfo&);
  static Style selectionPaintingStyle(LineLayoutItem,
                                      bool haveSelection,
                                      const PaintInfo&,
                                      const Style& textStyle);
  static Color textColorForWhiteBackground(Color);

  enum RotationDirection { Counterclockwise, Clockwise };
  static AffineTransform rotation(const LayoutRect& boxRect, RotationDirection);

 private:
  void updateGraphicsContext(const Style& style,
                             GraphicsContextStateSaver& saver) {
    updateGraphicsContext(m_graphicsContext, style, m_horizontal, saver);
  }

  enum PaintInternalStep { PaintText, PaintEmphasisMark };

  template <PaintInternalStep step>
  void paintInternalRun(TextRunPaintInfo&, unsigned from, unsigned to);

  template <PaintInternalStep step>
  void paintInternal(unsigned startOffset,
                     unsigned endOffset,
                     unsigned truncationPoint);

  void paintEmphasisMarkForCombinedText();

  GraphicsContext& m_graphicsContext;
  const Font& m_font;
  const TextRun& m_run;
  LayoutPoint m_textOrigin;
  LayoutRect m_textBounds;
  bool m_horizontal;
  AtomicString m_emphasisMark;
  int m_emphasisMarkOffset;
  LayoutTextCombine* m_combinedText;
  int m_ellipsisOffset;
};

inline AffineTransform TextPainter::rotation(
    const LayoutRect& boxRect,
    RotationDirection rotationDirection) {
  // Why this matrix is correct: consider the case of a clockwise rotation.

  // Let the corner points that define |boxRect| be ABCD, where A is top-left
  // and B is bottom-left.

  // 1. We want B to end up at the same pixel position after rotation as A is
  //    before rotation.
  // 2. Before rotation, B is at (x(), maxY())
  // 3. Rotating clockwise by 90 degrees places B at the coordinates
  //    (-maxY(), x()).
  // 4. Point A before rotation is at (x(), y())
  // 5. Therefore the translation from (3) to (4) is (x(), y()) - (-maxY(), x())
  //    = (x() + maxY(), y() - x())

  // A similar argument derives the counter-clockwise case.
  return rotationDirection == Clockwise
             ? AffineTransform(0, 1, -1, 0, boxRect.x() + boxRect.maxY(),
                               boxRect.y() - boxRect.x())
             : AffineTransform(0, -1, 1, 0, boxRect.x() - boxRect.y(),
                               boxRect.x() + boxRect.maxY());
}

}  // namespace blink

#endif  // TextPainter_h
