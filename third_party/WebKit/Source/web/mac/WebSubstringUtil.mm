/*
 * Copyright (C) 2005, 2007, 2008 Apple Inc. All rights reserved.
 * Copyright (C) 2011 Google Inc. All rights reserved.
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

#include "public/web/mac/WebSubstringUtil.h"

#import <Cocoa/Cocoa.h>

#include "bindings/core/v8/ExceptionState.h"
#include "core/dom/Document.h"
#include "core/dom/Element.h"
#include "core/dom/Node.h"
#include "core/dom/Range.h"
#include "core/editing/FrameSelection.h"
#include "core/editing/PlainTextRange.h"
#include "core/editing/iterators/TextIterator.h"
#include "core/frame/FrameView.h"
#include "core/frame/LocalFrame.h"
#include "core/frame/VisualViewport.h"
#include "core/html/HTMLElement.h"
#include "core/layout/HitTestResult.h"
#include "core/layout/LayoutObject.h"
#include "core/page/Page.h"
#include "core/style/ComputedStyle.h"
#include "platform/fonts/Font.h"
#include "platform/mac/ColorMac.h"
#include "public/platform/WebRect.h"
#include "public/web/WebFrameWidget.h"
#include "public/web/WebHitTestResult.h"
#include "public/web/WebLocalFrame.h"
#include "public/web/WebRange.h"
#include "web/WebFrameWidgetBase.h"
#include "web/WebLocalFrameImpl.h"

using namespace blink;

static NSAttributedString* attributedSubstringFromRange(
    const EphemeralRange& range,
    float fontScale) {
  NSMutableAttributedString* string = [[NSMutableAttributedString alloc] init];
  NSMutableDictionary* attrs = [NSMutableDictionary dictionary];
  size_t length = range.endPosition().computeOffsetInContainerNode() -
                  range.startPosition().computeOffsetInContainerNode();

  unsigned position = 0;

  // TODO(dglazkov): The use of updateStyleAndLayoutIgnorePendingStylesheets
  // needs to be audited.  see http://crbug.com/590369 for more details.
  range.startPosition()
      .document()
      ->updateStyleAndLayoutIgnorePendingStylesheets();

  for (TextIterator it(range.startPosition(), range.endPosition());
       !it.atEnd() && [string length] < length; it.advance()) {
    unsigned numCharacters = it.length();
    if (!numCharacters)
      continue;

    Node* container = it.currentContainer();
    LayoutObject* layoutObject = container->layoutObject();
    DCHECK(layoutObject);
    if (!layoutObject)
      continue;

    const ComputedStyle* style = layoutObject->style();
    FontPlatformData fontPlatformData =
        style->font().primaryFont()->platformData();
    fontPlatformData.m_textSize *= fontScale;
    NSFont* font = toNSFont(fontPlatformData.ctFont());
    // If the platform font can't be loaded, or the size is incorrect comparing
    // to the computed style, it's likely that the site is using a web font.
    // For now, just use the default font instead.
    // TODO(rsesek): Change the font activation flags to allow other processes
    // to use the font.
    // TODO(shuchen): Support scaling the font as necessary according to CSS
    // transforms, not just pinch-zoom.
    if (!font ||
        floor(fontPlatformData.size()) !=
            floor([[font fontDescriptor] pointSize])) {
      font = [NSFont
          systemFontOfSize:style->font().getFontDescription().computedSize() *
                           fontScale];
    }
    [attrs setObject:font forKey:NSFontAttributeName];

    if (style->visitedDependentColor(CSSPropertyColor).alpha())
      [attrs setObject:nsColor(style->visitedDependentColor(CSSPropertyColor))
                forKey:NSForegroundColorAttributeName];
    else
      [attrs removeObjectForKey:NSForegroundColorAttributeName];
    if (style->visitedDependentColor(CSSPropertyBackgroundColor).alpha())
      [attrs setObject:nsColor(style->visitedDependentColor(
                           CSSPropertyBackgroundColor))
                forKey:NSBackgroundColorAttributeName];
    else
      [attrs removeObjectForKey:NSBackgroundColorAttributeName];

    ForwardsTextBuffer characters;
    it.copyTextTo(&characters);
    NSString* substring =
        [[[NSString alloc] initWithCharacters:characters.data()
                                       length:characters.size()] autorelease];
    [string replaceCharactersInRange:NSMakeRange(position, 0)
                          withString:substring];
    [string setAttributes:attrs range:NSMakeRange(position, numCharacters)];
    position += numCharacters;
  }
  return [string autorelease];
}

WebPoint getBaselinePoint(FrameView* frameView,
                          const EphemeralRange& range,
                          NSAttributedString* string) {
  // TODO(yosin): We shold avoid to create |Range| object. See crbug.com/529985.
  IntRect stringRect =
      frameView->contentsToViewport(createRange(range)->boundingBox());
  IntPoint stringPoint = stringRect.minXMaxYCorner();

  // Adjust for the font's descender. AppKit wants the baseline point.
  if ([string length]) {
    NSDictionary* attributes = [string attributesAtIndex:0 effectiveRange:NULL];
    if (NSFont* font = [attributes objectForKey:NSFontAttributeName])
      stringPoint.move(0, ceil([font descender]));
  }
  return stringPoint;
}

namespace blink {

NSAttributedString* WebSubstringUtil::attributedWordAtPoint(
    WebFrameWidget* frameWidget,
    WebPoint point,
    WebPoint& baselinePoint) {
  HitTestResult result =
      static_cast<WebFrameWidgetBase*>(frameWidget)->coreHitTestResultAt(point);

  if (!result.innerNode())
    return nil;
  LocalFrame* frame = result.innerNode()->document().frame();
  EphemeralRange range =
      frame->rangeForPoint(result.roundedPointInInnerNodeFrame());
  if (range.isNull())
    return nil;

  // Expand to word under point.
  const VisibleSelection& selection =
      createVisibleSelection(SelectionInDOMTree::Builder()
                                 .setBaseAndExtent(range)
                                 .setGranularity(WordGranularity)
                                 .build());
  const EphemeralRange wordRange = selection.toNormalizedEphemeralRange();

  // Convert to NSAttributedString.
  NSAttributedString* string = attributedSubstringFromRange(
      wordRange, frame->page()->visualViewport().scale());
  baselinePoint = getBaselinePoint(frame->view(), wordRange, string);
  return string;
}

NSAttributedString* WebSubstringUtil::attributedSubstringInRange(
    WebLocalFrame* webFrame,
    size_t location,
    size_t length) {
  return WebSubstringUtil::attributedSubstringInRange(webFrame, location,
                                                      length, nil);
}

NSAttributedString* WebSubstringUtil::attributedSubstringInRange(
    WebLocalFrame* webFrame,
    size_t location,
    size_t length,
    WebPoint* baselinePoint) {
  LocalFrame* frame = toWebLocalFrameImpl(webFrame)->frame();
  if (frame->view()->needsLayout())
    frame->view()->layout();

  Element* editable = frame->selection().rootEditableElementOrDocumentElement();
  if (!editable)
    return nil;
  const EphemeralRange ephemeralRange(
      PlainTextRange(location, location + length).createRange(*editable));
  if (ephemeralRange.isNull())
    return nil;

  NSAttributedString* result = attributedSubstringFromRange(
      ephemeralRange, frame->page()->visualViewport().scale());
  if (baselinePoint)
    *baselinePoint = getBaselinePoint(frame->view(), ephemeralRange, result);
  return result;
}

}  // namespace blink
