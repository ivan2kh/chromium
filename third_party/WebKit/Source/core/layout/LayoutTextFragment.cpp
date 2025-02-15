/*
 * (C) 1999 Lars Knoll (knoll@kde.org)
 * (C) 2000 Dirk Mueller (mueller@kde.org)
 * Copyright (C) 2004, 2005, 2006, 2007 Apple Inc. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 */

#include "core/layout/LayoutTextFragment.h"

#include "core/dom/FirstLetterPseudoElement.h"
#include "core/dom/PseudoElement.h"
#include "core/dom/StyleChangeReason.h"
#include "core/dom/Text.h"
#include "core/frame/FrameView.h"
#include "core/layout/HitTestResult.h"

namespace blink {

LayoutTextFragment::LayoutTextFragment(Node* node,
                                       StringImpl* str,
                                       int startOffset,
                                       int length)
    : LayoutText(node,
                 str ? str->substring(startOffset, length)
                     : PassRefPtr<StringImpl>(nullptr)),
      m_start(startOffset),
      m_fragmentLength(length),
      m_isRemainingTextLayoutObject(false),
      m_contentString(str),
      m_firstLetterPseudoElement(nullptr) {}

LayoutTextFragment::LayoutTextFragment(Node* node, StringImpl* str)
    : LayoutTextFragment(node, str, 0, str ? str->length() : 0) {}

LayoutTextFragment::~LayoutTextFragment() {
  ASSERT(!m_firstLetterPseudoElement);
}

LayoutTextFragment* LayoutTextFragment::createAnonymous(PseudoElement& pseudo,
                                                        StringImpl* text,
                                                        unsigned start,
                                                        unsigned length) {
  LayoutTextFragment* fragment =
      new LayoutTextFragment(nullptr, text, start, length);
  fragment->setDocumentForAnonymous(&pseudo.document());
  if (length)
    pseudo.document().view()->incrementVisuallyNonEmptyCharacterCount(length);
  return fragment;
}

LayoutTextFragment* LayoutTextFragment::createAnonymous(PseudoElement& pseudo,
                                                        StringImpl* text) {
  return createAnonymous(pseudo, text, 0, text ? text->length() : 0);
}

void LayoutTextFragment::willBeDestroyed() {
  if (m_isRemainingTextLayoutObject && m_firstLetterPseudoElement)
    m_firstLetterPseudoElement->setRemainingTextLayoutObject(nullptr);
  m_firstLetterPseudoElement = nullptr;
  LayoutText::willBeDestroyed();
}

PassRefPtr<StringImpl> LayoutTextFragment::completeText() const {
  Text* text = associatedTextNode();
  return text ? text->dataImpl() : contentString();
}

void LayoutTextFragment::setContentString(StringImpl* str) {
  m_contentString = str;
  setText(str);
}

PassRefPtr<StringImpl> LayoutTextFragment::originalText() const {
  RefPtr<StringImpl> result = completeText();
  if (!result)
    return nullptr;
  return result->substring(start(), fragmentLength());
}

void LayoutTextFragment::setText(PassRefPtr<StringImpl> text, bool force) {
  LayoutText::setText(std::move(text), force);

  m_start = 0;
  m_fragmentLength = textLength();

  // If we're the remaining text from a first letter then we have to tell the
  // first letter pseudo element to reattach itself so it can re-calculate the
  // correct first-letter settings.
  if (isRemainingTextLayoutObject()) {
    ASSERT(firstLetterPseudoElement());
    firstLetterPseudoElement()->updateTextFragments();
  }
}

void LayoutTextFragment::setTextFragment(PassRefPtr<StringImpl> text,
                                         unsigned start,
                                         unsigned length) {
  LayoutText::setText(std::move(text), false);

  m_start = start;
  m_fragmentLength = length;
}

void LayoutTextFragment::transformText() {
  // Note, we have to call LayoutText::setText here because, if we use our
  // version we will, potentially, screw up the first-letter settings where
  // we only use portions of the string.
  if (RefPtr<StringImpl> textToTransform = originalText())
    LayoutText::setText(std::move(textToTransform), true);
}

UChar LayoutTextFragment::previousCharacter() const {
  if (start()) {
    StringImpl* original = completeText().get();
    if (original && start() <= original->length())
      return (*original)[start() - 1];
  }

  return LayoutText::previousCharacter();
}

// If this is the layoutObject for a first-letter pseudoNode then we have to
// look at the node for the remaining text to find our content.
Text* LayoutTextFragment::associatedTextNode() const {
  Node* node = this->firstLetterPseudoElement();
  if (m_isRemainingTextLayoutObject || !node) {
    // If we don't have a node, then we aren't part of a first-letter pseudo
    // element, so use the actual node. Likewise, if we have a node, but
    // we're the remainingTextLayoutObject for a pseudo element use the real
    // text node.
    node = this->node();
  }

  if (!node)
    return nullptr;

  if (node->isFirstLetterPseudoElement()) {
    FirstLetterPseudoElement* pseudo = toFirstLetterPseudoElement(node);
    LayoutObject* nextLayoutObject =
        FirstLetterPseudoElement::firstLetterTextLayoutObject(*pseudo);
    if (!nextLayoutObject)
      return nullptr;
    node = nextLayoutObject->node();
  }
  return (node && node->isTextNode()) ? toText(node) : nullptr;
}

void LayoutTextFragment::updateHitTestResult(HitTestResult& result,
                                             const LayoutPoint& point) {
  if (result.innerNode())
    return;

  LayoutObject::updateHitTestResult(result, point);

  // If we aren't part of a first-letter element, or if we
  // are part of first-letter but we're the remaining text then return.
  if (m_isRemainingTextLayoutObject || !firstLetterPseudoElement())
    return;
  result.setInnerNode(firstLetterPseudoElement());
}

}  // namespace blink
