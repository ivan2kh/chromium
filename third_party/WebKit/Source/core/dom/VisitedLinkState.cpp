/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 2004-2005 Allan Sandfeld Jensen (kde@carewolf.com)
 * Copyright (C) 2006, 2007 Nicholas Shanks (webkit@nickshanks.com)
 * Copyright (C) 2005, 2006, 2007, 2008, 2009, 2010, 2011 Apple Inc. All rights
 * reserved.
 * Copyright (C) 2007 Alexey Proskuryakov <ap@webkit.org>
 * Copyright (C) 2007, 2008 Eric Seidel <eric@webkit.org>
 * Copyright (C) 2008, 2009 Torch Mobile Inc. All rights reserved.
 * (http://www.torchmobile.com/)
 * Copyright (c) 2011, Code Aurora Forum. All rights reserved.
 * Copyright (C) Research In Motion Limited 2011. All rights reserved.
 * Copyright (C) 2012 Google Inc. All rights reserved.
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
 */

#include "core/dom/VisitedLinkState.h"

#include "core/HTMLNames.h"
#include "core/dom/ElementTraversal.h"
#include "core/dom/shadow/ElementShadow.h"
#include "core/dom/shadow/ShadowRoot.h"
#include "core/html/HTMLAnchorElement.h"
#include "core/svg/SVGURIReference.h"
#include "public/platform/Platform.h"

namespace blink {

static inline const AtomicString& linkAttribute(const Element& element) {
  DCHECK(element.isLink());
  if (element.isHTMLElement())
    return element.fastGetAttribute(HTMLNames::hrefAttr);
  DCHECK(element.isSVGElement());
  return SVGURIReference::legacyHrefString(toSVGElement(element));
}

static inline LinkHash linkHashForElement(
    const Element& element,
    const AtomicString& attribute = AtomicString()) {
  DCHECK(attribute.isNull() || linkAttribute(element) == attribute);
  if (isHTMLAnchorElement(element))
    return toHTMLAnchorElement(element).visitedLinkHash();
  return visitedLinkHash(
      element.document().baseURL(),
      attribute.isNull() ? linkAttribute(element) : attribute);
}

VisitedLinkState::VisitedLinkState(const Document& document)
    : m_document(document) {}

static void invalidateStyleForAllLinksRecursively(
    Node& rootNode,
    bool invalidateVisitedLinkHashes) {
  for (Node& node : NodeTraversal::startsAt(rootNode)) {
    if (node.isLink()) {
      if (invalidateVisitedLinkHashes && isHTMLAnchorElement(node))
        toHTMLAnchorElement(node).invalidateCachedVisitedLinkHash();
      toElement(node).pseudoStateChanged(CSSSelector::PseudoLink);
      toElement(node).pseudoStateChanged(CSSSelector::PseudoVisited);
      toElement(node).pseudoStateChanged(CSSSelector::PseudoAnyLink);
    }
    if (isShadowHost(&node)) {
      for (ShadowRoot* root = node.youngestShadowRoot(); root;
           root = root->olderShadowRoot())
        invalidateStyleForAllLinksRecursively(*root,
                                              invalidateVisitedLinkHashes);
    }
  }
}

void VisitedLinkState::invalidateStyleForAllLinks(
    bool invalidateVisitedLinkHashes) {
  if (!m_linksCheckedForVisitedState.isEmpty() && document().firstChild())
    invalidateStyleForAllLinksRecursively(*document().firstChild(),
                                          invalidateVisitedLinkHashes);
}

static void invalidateStyleForLinkRecursively(Node& rootNode,
                                              LinkHash linkHash) {
  for (Node& node : NodeTraversal::startsAt(rootNode)) {
    if (node.isLink() && linkHashForElement(toElement(node)) == linkHash) {
      toElement(node).pseudoStateChanged(CSSSelector::PseudoLink);
      toElement(node).pseudoStateChanged(CSSSelector::PseudoVisited);
      toElement(node).pseudoStateChanged(CSSSelector::PseudoAnyLink);
    }
    if (isShadowHost(&node))
      for (ShadowRoot* root = node.youngestShadowRoot(); root;
           root = root->olderShadowRoot())
        invalidateStyleForLinkRecursively(*root, linkHash);
  }
}

void VisitedLinkState::invalidateStyleForLink(LinkHash linkHash) {
  if (m_linksCheckedForVisitedState.contains(linkHash) &&
      document().firstChild())
    invalidateStyleForLinkRecursively(*document().firstChild(), linkHash);
}

EInsideLink VisitedLinkState::determineLinkStateSlowCase(
    const Element& element) {
  DCHECK(element.isLink());
  DCHECK(document().isActive());
  DCHECK(document() == element.document());

  const AtomicString& attribute = linkAttribute(element);

  if (attribute.isNull())
    return EInsideLink::kNotInsideLink;  // This can happen for <img usemap>

  // An empty attribute refers to the document itself which is always
  // visited. It is useful to check this explicitly so that visited
  // links can be tested in platform independent manner, without
  // explicit support in the test harness.
  if (attribute.isEmpty())
    return EInsideLink::kInsideVisitedLink;

  if (LinkHash hash = linkHashForElement(element, attribute)) {
    m_linksCheckedForVisitedState.insert(hash);
    if (Platform::current()->isLinkVisited(hash))
      return EInsideLink::kInsideVisitedLink;
  }

  return EInsideLink::kInsideUnvisitedLink;
}

DEFINE_TRACE(VisitedLinkState) {
  visitor->trace(m_document);
}

}  // namespace blink
