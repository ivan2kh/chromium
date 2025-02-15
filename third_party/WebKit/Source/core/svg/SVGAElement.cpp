/*
 * Copyright (C) 2004, 2005, 2008 Nikolas Zimmermann <zimmermann@kde.org>
 * Copyright (C) 2004, 2005, 2007 Rob Buis <buis@kde.org>
 * Copyright (C) 2007 Eric Seidel <eric@webkit.org>
 * Copyright (C) 2010 Apple Inc. All rights reserved.
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

#include "core/svg/SVGAElement.h"

#include "core/SVGNames.h"
#include "core/XLinkNames.h"
#include "core/dom/Attr.h"
#include "core/dom/Attribute.h"
#include "core/dom/Document.h"
#include "core/editing/EditingUtilities.h"
#include "core/events/KeyboardEvent.h"
#include "core/events/MouseEvent.h"
#include "core/frame/LocalFrame.h"
#include "core/html/HTMLAnchorElement.h"
#include "core/html/HTMLFormElement.h"
#include "core/html/parser/HTMLParserIdioms.h"
#include "core/layout/svg/LayoutSVGInline.h"
#include "core/layout/svg/LayoutSVGText.h"
#include "core/layout/svg/LayoutSVGTransformableContainer.h"
#include "core/loader/FrameLoadRequest.h"
#include "core/loader/FrameLoader.h"
#include "core/loader/FrameLoaderTypes.h"
#include "core/page/ChromeClient.h"
#include "core/page/Page.h"
#include "core/svg/animation/SVGSMILElement.h"
#include "platform/loader/fetch/ResourceRequest.h"

namespace blink {

using namespace HTMLNames;

inline SVGAElement::SVGAElement(Document& document)
    : SVGGraphicsElement(SVGNames::aTag, document),
      SVGURIReference(this),
      m_svgTarget(SVGAnimatedString::create(this, SVGNames::targetAttr)),
      m_wasFocusedByMouse(false) {
  addToPropertyMap(m_svgTarget);
}

DEFINE_TRACE(SVGAElement) {
  visitor->trace(m_svgTarget);
  SVGGraphicsElement::trace(visitor);
  SVGURIReference::trace(visitor);
}

DEFINE_NODE_FACTORY(SVGAElement)

String SVGAElement::title() const {
  // If the xlink:title is set (non-empty string), use it.
  const AtomicString& title = fastGetAttribute(XLinkNames::titleAttr);
  if (!title.isEmpty())
    return title;

  // Otherwise, use the title of this element.
  return SVGElement::title();
}

void SVGAElement::svgAttributeChanged(const QualifiedName& attrName) {
  // Unlike other SVG*Element classes, SVGAElement only listens to
  // SVGURIReference changes as none of the other properties changes the linking
  // behaviour for our <a> element.
  if (SVGURIReference::isKnownAttribute(attrName)) {
    SVGElement::InvalidationGuard invalidationGuard(this);

    bool wasLink = isLink();
    setIsLink(!hrefString().isNull());

    if (wasLink || isLink()) {
      pseudoStateChanged(CSSSelector::PseudoLink);
      pseudoStateChanged(CSSSelector::PseudoVisited);
      pseudoStateChanged(CSSSelector::PseudoAnyLink);
    }
    return;
  }

  SVGGraphicsElement::svgAttributeChanged(attrName);
}

LayoutObject* SVGAElement::createLayoutObject(const ComputedStyle&) {
  if (parentNode() && parentNode()->isSVGElement() &&
      toSVGElement(parentNode())->isTextContent())
    return new LayoutSVGInline(this);

  return new LayoutSVGTransformableContainer(this);
}

void SVGAElement::defaultEventHandler(Event* event) {
  if (isLink()) {
    if (isFocused() && isEnterKeyKeydownEvent(event)) {
      event->setDefaultHandled();
      dispatchSimulatedClick(event);
      return;
    }

    if (isLinkClick(event)) {
      String url = stripLeadingAndTrailingHTMLSpaces(hrefString());

      if (url[0] == '#') {
        Element* targetElement =
            treeScope().getElementById(AtomicString(url.substring(1)));
        if (targetElement && isSVGSMILElement(*targetElement)) {
          toSVGSMILElement(targetElement)->beginByLinkActivation();
          event->setDefaultHandled();
          return;
        }
      }

      AtomicString target(m_svgTarget->currentValue()->value());
      if (target.isEmpty() && fastGetAttribute(XLinkNames::showAttr) == "new")
        target = AtomicString("_blank");
      event->setDefaultHandled();

      LocalFrame* frame = document().frame();
      if (!frame)
        return;
      FrameLoadRequest frameRequest(
          &document(), ResourceRequest(document().completeURL(url)), target);
      frameRequest.setTriggeringEvent(event);
      frame->loader().load(frameRequest);
      return;
    }
  }

  SVGGraphicsElement::defaultEventHandler(event);
}

int SVGAElement::tabIndex() const {
  // Skip the supportsFocus check in SVGElement.
  return Element::tabIndex();
}

bool SVGAElement::supportsFocus() const {
  if (hasEditableStyle(*this))
    return SVGGraphicsElement::supportsFocus();
  // If not a link we should still be able to focus the element if it has
  // tabIndex.
  return isLink() || SVGGraphicsElement::supportsFocus();
}

bool SVGAElement::shouldHaveFocusAppearance() const {
  return !m_wasFocusedByMouse || SVGGraphicsElement::supportsFocus();
}

// TODO(lanwei): Will add the InputDeviceCapabilities when SVGAElement gets
// focus later, see https://crbug.com/476530.
void SVGAElement::dispatchFocusEvent(
    Element* oldFocusedElement,
    WebFocusType type,
    InputDeviceCapabilities* sourceCapabilities) {
  if (type != WebFocusTypePage)
    m_wasFocusedByMouse = type == WebFocusTypeMouse;
  SVGGraphicsElement::dispatchFocusEvent(oldFocusedElement, type,
                                         sourceCapabilities);
}

void SVGAElement::dispatchBlurEvent(
    Element* newFocusedElement,
    WebFocusType type,
    InputDeviceCapabilities* sourceCapabilities) {
  if (type != WebFocusTypePage)
    m_wasFocusedByMouse = false;
  SVGGraphicsElement::dispatchBlurEvent(newFocusedElement, type,
                                        sourceCapabilities);
}

bool SVGAElement::isURLAttribute(const Attribute& attribute) const {
  return attribute.name().localName() == hrefAttr ||
         SVGGraphicsElement::isURLAttribute(attribute);
}

bool SVGAElement::isMouseFocusable() const {
  if (isLink())
    return supportsFocus();

  return SVGElement::isMouseFocusable();
}

bool SVGAElement::isKeyboardFocusable() const {
  if (isFocusable() && Element::supportsFocus())
    return SVGElement::isKeyboardFocusable();
  if (isLink() && !document().page()->chromeClient().tabsToLinks())
    return false;
  return SVGElement::isKeyboardFocusable();
}

bool SVGAElement::canStartSelection() const {
  if (!isLink())
    return SVGElement::canStartSelection();
  return hasEditableStyle(*this);
}

bool SVGAElement::willRespondToMouseClickEvents() {
  return isLink() || SVGGraphicsElement::willRespondToMouseClickEvents();
}

}  // namespace blink
