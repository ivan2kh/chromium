/*
 * Copyright (C) 2006 Nikolas Zimmermann <zimmermann@kde.org>
 * Copyright (C) Research In Motion Limited 2010. All rights reserved.
 * Copyright (C) 2012 Adobe Systems Incorporated. All rights reserved.
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

#include "core/layout/svg/LayoutSVGResourceRadialGradient.h"

#include "core/svg/SVGRadialGradientElement.h"

namespace blink {

LayoutSVGResourceRadialGradient::LayoutSVGResourceRadialGradient(
    SVGRadialGradientElement* node)
    : LayoutSVGResourceGradient(node),
      m_attributesWrapper(RadialGradientAttributesWrapper::create()) {}

LayoutSVGResourceRadialGradient::~LayoutSVGResourceRadialGradient() {}

bool LayoutSVGResourceRadialGradient::collectGradientAttributes() {
  DCHECK(element());
  m_attributesWrapper->set(RadialGradientAttributes());
  return toSVGRadialGradientElement(element())->collectGradientAttributes(
      mutableAttributes());
}

FloatPoint LayoutSVGResourceRadialGradient::centerPoint(
    const RadialGradientAttributes& attributes) const {
  return SVGLengthContext::resolvePoint(element(), attributes.gradientUnits(),
                                        *attributes.cx(), *attributes.cy());
}

FloatPoint LayoutSVGResourceRadialGradient::focalPoint(
    const RadialGradientAttributes& attributes) const {
  return SVGLengthContext::resolvePoint(element(), attributes.gradientUnits(),
                                        *attributes.fx(), *attributes.fy());
}

float LayoutSVGResourceRadialGradient::radius(
    const RadialGradientAttributes& attributes) const {
  return SVGLengthContext::resolveLength(element(), attributes.gradientUnits(),
                                         *attributes.r());
}

float LayoutSVGResourceRadialGradient::focalRadius(
    const RadialGradientAttributes& attributes) const {
  return SVGLengthContext::resolveLength(element(), attributes.gradientUnits(),
                                         *attributes.fr());
}

PassRefPtr<Gradient> LayoutSVGResourceRadialGradient::buildGradient() const {
  const RadialGradientAttributes& attributes = this->attributes();
  RefPtr<Gradient> gradient = Gradient::create(
      focalPoint(attributes), focalRadius(attributes), centerPoint(attributes),
      radius(attributes), 1,
      platformSpreadMethodFromSVGType(attributes.spreadMethod()),
      Gradient::ColorInterpolation::Unpremultiplied);
  gradient->addColorStops(attributes.stops());
  return gradient.release();
}

}  // namespace blink
