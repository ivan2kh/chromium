/*
 * Copyright (C) 2004, 2005, 2006, 2008 Nikolas Zimmermann <zimmermann@kde.org>
 * Copyright (C) 2004, 2005, 2006 Rob Buis <buis@kde.org>
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

#ifndef SVGGradientElement_h
#define SVGGradientElement_h

#include "core/SVGNames.h"
#include "core/svg/SVGAnimatedEnumeration.h"
#include "core/svg/SVGAnimatedTransformList.h"
#include "core/svg/SVGElement.h"
#include "core/svg/SVGURIReference.h"
#include "core/svg/SVGUnitTypes.h"
#include "platform/graphics/Gradient.h"
#include "platform/heap/Handle.h"

namespace blink {

struct GradientAttributes;

enum SVGSpreadMethodType {
  SVGSpreadMethodUnknown = 0,
  SVGSpreadMethodPad,
  SVGSpreadMethodReflect,
  SVGSpreadMethodRepeat
};
template <>
const SVGEnumerationStringEntries&
getStaticStringEntries<SVGSpreadMethodType>();

class SVGGradientElement : public SVGElement, public SVGURIReference {
  DEFINE_WRAPPERTYPEINFO();
  USING_GARBAGE_COLLECTED_MIXIN(SVGGradientElement);

 public:
  SVGAnimatedTransformList* gradientTransform() const {
    return m_gradientTransform.get();
  }
  SVGAnimatedEnumeration<SVGSpreadMethodType>* spreadMethod() const {
    return m_spreadMethod.get();
  }
  SVGAnimatedEnumeration<SVGUnitTypes::SVGUnitType>* gradientUnits() const {
    return m_gradientUnits.get();
  }

  const SVGGradientElement* referencedElement() const;
  void collectCommonAttributes(GradientAttributes&) const;

  DECLARE_VIRTUAL_TRACE();

 protected:
  SVGGradientElement(const QualifiedName&, Document&);

  using VisitedSet = HeapHashSet<Member<const SVGGradientElement>>;

  void svgAttributeChanged(const QualifiedName&) override;

 private:
  bool needsPendingResourceHandling() const final { return false; }

  void collectStyleForPresentationAttribute(const QualifiedName&,
                                            const AtomicString&,
                                            MutableStylePropertySet*) override;

  void childrenChanged(const ChildrenChange&) final;

  Vector<Gradient::ColorStop> buildStops() const;

  Member<SVGAnimatedTransformList> m_gradientTransform;
  Member<SVGAnimatedEnumeration<SVGSpreadMethodType>> m_spreadMethod;
  Member<SVGAnimatedEnumeration<SVGUnitTypes::SVGUnitType>> m_gradientUnits;
};

inline bool isSVGGradientElement(const SVGElement& element) {
  return element.hasTagName(SVGNames::radialGradientTag) ||
         element.hasTagName(SVGNames::linearGradientTag);
}

DEFINE_SVGELEMENT_TYPE_CASTS_WITH_FUNCTION(SVGGradientElement);

}  // namespace blink

#endif  // SVGGradientElement_h
