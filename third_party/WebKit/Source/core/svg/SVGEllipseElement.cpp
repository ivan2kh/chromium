/*
 * Copyright (C) 2004, 2005, 2006, 2008 Nikolas Zimmermann <zimmermann@kde.org>
 * Copyright (C) 2004, 2005, 2006, 2007 Rob Buis <buis@kde.org>
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

#include "core/svg/SVGEllipseElement.h"

#include "core/dom/StyleChangeReason.h"
#include "core/layout/svg/LayoutSVGEllipse.h"
#include "core/svg/SVGLength.h"

namespace blink {

inline SVGEllipseElement::SVGEllipseElement(Document& document)
    : SVGGeometryElement(SVGNames::ellipseTag, document),
      m_cx(SVGAnimatedLength::create(this,
                                     SVGNames::cxAttr,
                                     SVGLength::create(SVGLengthMode::Width),
                                     CSSPropertyCx)),
      m_cy(SVGAnimatedLength::create(this,
                                     SVGNames::cyAttr,
                                     SVGLength::create(SVGLengthMode::Height),
                                     CSSPropertyCy)),
      m_rx(SVGAnimatedLength::create(this,
                                     SVGNames::rxAttr,
                                     SVGLength::create(SVGLengthMode::Width),
                                     CSSPropertyRx)),
      m_ry(SVGAnimatedLength::create(this,
                                     SVGNames::ryAttr,
                                     SVGLength::create(SVGLengthMode::Height),
                                     CSSPropertyRy)) {
  addToPropertyMap(m_cx);
  addToPropertyMap(m_cy);
  addToPropertyMap(m_rx);
  addToPropertyMap(m_ry);
}

DEFINE_TRACE(SVGEllipseElement) {
  visitor->trace(m_cx);
  visitor->trace(m_cy);
  visitor->trace(m_rx);
  visitor->trace(m_ry);
  SVGGeometryElement::trace(visitor);
}

DEFINE_NODE_FACTORY(SVGEllipseElement)

Path SVGEllipseElement::asPath() const {
  Path path;

  SVGLengthContext lengthContext(this);
  DCHECK(layoutObject());
  const ComputedStyle& style = layoutObject()->styleRef();
  const SVGComputedStyle& svgStyle = style.svgStyle();

  float rx =
      lengthContext.valueForLength(svgStyle.rx(), style, SVGLengthMode::Width);
  if (rx < 0)
    return path;
  float ry =
      lengthContext.valueForLength(svgStyle.ry(), style, SVGLengthMode::Height);
  if (ry < 0)
    return path;
  if (!rx && !ry)
    return path;

  path.addEllipse(FloatRect(
      lengthContext.valueForLength(svgStyle.cx(), style, SVGLengthMode::Width) -
          rx,
      lengthContext.valueForLength(svgStyle.cy(), style,
                                   SVGLengthMode::Height) -
          ry,
      rx * 2, ry * 2));

  return path;
}

void SVGEllipseElement::collectStyleForPresentationAttribute(
    const QualifiedName& name,
    const AtomicString& value,
    MutableStylePropertySet* style) {
  SVGAnimatedPropertyBase* property = propertyFromAttribute(name);
  if (property == m_cx) {
    addPropertyToPresentationAttributeStyle(style, property->cssPropertyId(),
                                            m_cx->cssValue());
  } else if (property == m_cy) {
    addPropertyToPresentationAttributeStyle(style, property->cssPropertyId(),
                                            m_cy->cssValue());
  } else if (property == m_rx) {
    addPropertyToPresentationAttributeStyle(style, property->cssPropertyId(),
                                            m_rx->cssValue());
  } else if (property == m_ry) {
    addPropertyToPresentationAttributeStyle(style, property->cssPropertyId(),
                                            m_ry->cssValue());
  } else {
    SVGGeometryElement::collectStyleForPresentationAttribute(name, value,
                                                             style);
  }
}

void SVGEllipseElement::svgAttributeChanged(const QualifiedName& attrName) {
  if (attrName == SVGNames::cxAttr || attrName == SVGNames::cyAttr ||
      attrName == SVGNames::rxAttr || attrName == SVGNames::ryAttr) {
    SVGElement::InvalidationGuard invalidationGuard(this);

    invalidateSVGPresentationAttributeStyle();
    setNeedsStyleRecalc(LocalStyleChange,
                        StyleChangeReasonForTracing::fromAttribute(attrName));
    updateRelativeLengthsInformation();

    LayoutSVGShape* layoutObject = toLayoutSVGShape(this->layoutObject());
    if (!layoutObject)
      return;

    layoutObject->setNeedsShapeUpdate();
    markForLayoutAndParentResourceInvalidation(layoutObject);
    return;
  }

  SVGGeometryElement::svgAttributeChanged(attrName);
}

bool SVGEllipseElement::selfHasRelativeLengths() const {
  return m_cx->currentValue()->isRelative() ||
         m_cy->currentValue()->isRelative() ||
         m_rx->currentValue()->isRelative() ||
         m_ry->currentValue()->isRelative();
}

LayoutObject* SVGEllipseElement::createLayoutObject(const ComputedStyle&) {
  return new LayoutSVGEllipse(this);
}

}  // namespace blink
