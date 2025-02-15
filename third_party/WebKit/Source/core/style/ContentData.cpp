/*
 * Copyright (C) 1999 Antti Koivisto (koivisto@kde.org)
 * Copyright (C) 2004, 2005, 2006, 2007, 2008, 2009, 2010 Apple Inc. All rights
 * reserved.
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

#include "core/style/ContentData.h"

#include <memory>
#include "core/dom/PseudoElement.h"
#include "core/layout/LayoutCounter.h"
#include "core/layout/LayoutImage.h"
#include "core/layout/LayoutImageResource.h"
#include "core/layout/LayoutImageResourceStyleImage.h"
#include "core/layout/LayoutQuote.h"
#include "core/layout/LayoutTextFragment.h"
#include "core/style/ComputedStyle.h"

namespace blink {

ContentData* ContentData::create(StyleImage* image) {
  return new ImageContentData(image);
}

ContentData* ContentData::create(const String& text) {
  return new TextContentData(text);
}

ContentData* ContentData::create(std::unique_ptr<CounterContent> counter) {
  return new CounterContentData(std::move(counter));
}

ContentData* ContentData::create(QuoteType quote) {
  return new QuoteContentData(quote);
}

ContentData* ContentData::clone() const {
  ContentData* result = cloneInternal();

  ContentData* lastNewData = result;
  for (const ContentData* contentData = next(); contentData;
       contentData = contentData->next()) {
    ContentData* newData = contentData->cloneInternal();
    lastNewData->setNext(newData);
    lastNewData = lastNewData->next();
  }

  return result;
}

DEFINE_TRACE(ContentData) {
  visitor->trace(m_next);
}

LayoutObject* ImageContentData::createLayoutObject(
    PseudoElement& pseudo,
    ComputedStyle& pseudoStyle) const {
  LayoutImage* image = LayoutImage::createAnonymous(pseudo);
  image->setPseudoStyle(&pseudoStyle);
  if (m_image)
    image->setImageResource(
        LayoutImageResourceStyleImage::create(m_image.get()));
  else
    image->setImageResource(LayoutImageResource::create());
  return image;
}

DEFINE_TRACE(ImageContentData) {
  visitor->trace(m_image);
  ContentData::trace(visitor);
}

LayoutObject* TextContentData::createLayoutObject(
    PseudoElement& pseudo,
    ComputedStyle& pseudoStyle) const {
  LayoutObject* layoutObject =
      LayoutTextFragment::createAnonymous(pseudo, m_text.impl());
  layoutObject->setPseudoStyle(&pseudoStyle);
  return layoutObject;
}

LayoutObject* CounterContentData::createLayoutObject(
    PseudoElement& pseudo,
    ComputedStyle& pseudoStyle) const {
  LayoutObject* layoutObject = new LayoutCounter(pseudo, *m_counter);
  layoutObject->setPseudoStyle(&pseudoStyle);
  return layoutObject;
}

LayoutObject* QuoteContentData::createLayoutObject(
    PseudoElement& pseudo,
    ComputedStyle& pseudoStyle) const {
  LayoutObject* layoutObject = new LayoutQuote(pseudo, m_quote);
  layoutObject->setPseudoStyle(&pseudoStyle);
  return layoutObject;
}

}  // namespace blink
