/*
 * Copyright (C) 2004, 2005, 2006, 2008 Nikolas Zimmermann <zimmermann@kde.org>
 * Copyright (C) 2004, 2005, 2006, 2007 Rob Buis <buis@kde.org>
 * Copyright (C) Research In Motion Limited 2010. All rights reserved.
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

#include "core/svg/SVGGradientElement.h"

#include "core/dom/Attribute.h"
#include "core/dom/ElementTraversal.h"
#include "core/dom/StyleChangeReason.h"
#include "core/layout/svg/LayoutSVGResourceContainer.h"
#include "core/svg/GradientAttributes.h"
#include "core/svg/SVGStopElement.h"
#include "core/svg/SVGTransformList.h"

namespace blink {

template <>
const SVGEnumerationStringEntries&
getStaticStringEntries<SVGSpreadMethodType>() {
  DEFINE_STATIC_LOCAL(SVGEnumerationStringEntries, entries, ());
  if (entries.isEmpty()) {
    entries.push_back(std::make_pair(SVGSpreadMethodPad, "pad"));
    entries.push_back(std::make_pair(SVGSpreadMethodReflect, "reflect"));
    entries.push_back(std::make_pair(SVGSpreadMethodRepeat, "repeat"));
  }
  return entries;
}

SVGGradientElement::SVGGradientElement(const QualifiedName& tagName,
                                       Document& document)
    : SVGElement(tagName, document),
      SVGURIReference(this),
      m_gradientTransform(
          SVGAnimatedTransformList::create(this,
                                           SVGNames::gradientTransformAttr,
                                           CSSPropertyTransform)),
      m_spreadMethod(SVGAnimatedEnumeration<SVGSpreadMethodType>::create(
          this,
          SVGNames::spreadMethodAttr,
          SVGSpreadMethodPad)),
      m_gradientUnits(SVGAnimatedEnumeration<SVGUnitTypes::SVGUnitType>::create(
          this,
          SVGNames::gradientUnitsAttr,
          SVGUnitTypes::kSvgUnitTypeObjectboundingbox)) {
  addToPropertyMap(m_gradientTransform);
  addToPropertyMap(m_spreadMethod);
  addToPropertyMap(m_gradientUnits);
}

DEFINE_TRACE(SVGGradientElement) {
  visitor->trace(m_gradientTransform);
  visitor->trace(m_spreadMethod);
  visitor->trace(m_gradientUnits);
  SVGElement::trace(visitor);
  SVGURIReference::trace(visitor);
}

void SVGGradientElement::collectStyleForPresentationAttribute(
    const QualifiedName& name,
    const AtomicString& value,
    MutableStylePropertySet* style) {
  if (name == SVGNames::gradientTransformAttr) {
    addPropertyToPresentationAttributeStyle(
        style, CSSPropertyTransform,
        m_gradientTransform->currentValue()->cssValue());
    return;
  }
  SVGElement::collectStyleForPresentationAttribute(name, value, style);
}

void SVGGradientElement::svgAttributeChanged(const QualifiedName& attrName) {
  if (attrName == SVGNames::gradientTransformAttr) {
    invalidateSVGPresentationAttributeStyle();
    setNeedsStyleRecalc(LocalStyleChange,
                        StyleChangeReasonForTracing::fromAttribute(attrName));
  }

  if (attrName == SVGNames::gradientUnitsAttr ||
      attrName == SVGNames::gradientTransformAttr ||
      attrName == SVGNames::spreadMethodAttr ||
      SVGURIReference::isKnownAttribute(attrName)) {
    SVGElement::InvalidationGuard invalidationGuard(this);

    LayoutSVGResourceContainer* layoutObject =
        toLayoutSVGResourceContainer(this->layoutObject());
    if (layoutObject)
      layoutObject->invalidateCacheAndMarkForLayout();

    return;
  }

  SVGElement::svgAttributeChanged(attrName);
}

void SVGGradientElement::childrenChanged(const ChildrenChange& change) {
  SVGElement::childrenChanged(change);

  if (change.byParser)
    return;

  if (LayoutObject* object = layoutObject())
    object->setNeedsLayoutAndFullPaintInvalidation(
        LayoutInvalidationReason::ChildChanged);
}

void SVGGradientElement::collectCommonAttributes(
    GradientAttributes& attributes) const {
  if (!attributes.hasSpreadMethod() && spreadMethod()->isSpecified())
    attributes.setSpreadMethod(spreadMethod()->currentValue()->enumValue());

  if (!attributes.hasGradientUnits() && gradientUnits()->isSpecified())
    attributes.setGradientUnits(gradientUnits()->currentValue()->enumValue());

  if (!attributes.hasGradientTransform() &&
      hasTransform(SVGElement::ExcludeMotionTransform)) {
    attributes.setGradientTransform(
        calculateTransform(SVGElement::ExcludeMotionTransform));
  }

  if (!attributes.hasStops()) {
    const Vector<Gradient::ColorStop>& stops(buildStops());
    if (!stops.isEmpty())
      attributes.setStops(stops);
  }
}

const SVGGradientElement* SVGGradientElement::referencedElement() const {
  // Respect xlink:href, take attributes from referenced element.
  Element* referencedElement =
      targetElementFromIRIString(hrefString(), treeScope());
  if (!referencedElement || !isSVGGradientElement(*referencedElement))
    return nullptr;
  return toSVGGradientElement(referencedElement);
}

Vector<Gradient::ColorStop> SVGGradientElement::buildStops() const {
  Vector<Gradient::ColorStop> stops;

  float previousOffset = 0.0f;
  for (const SVGStopElement& stop :
       Traversal<SVGStopElement>::childrenOf(*this)) {
    // Figure out right monotonic offset.
    float offset = stop.offset()->currentValue()->value();
    offset = std::min(std::max(previousOffset, offset), 1.0f);
    previousOffset = offset;

    stops.push_back(
        Gradient::ColorStop(offset, stop.stopColorIncludingOpacity()));
  }
  return stops;
}

}  // namespace blink
