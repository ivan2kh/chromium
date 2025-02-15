/*
 * Copyright (C) 2002, 2003 The Karbon Developers
 * Copyright (C) 2006 Alexander Kellett <lypanov@kde.org>
 * Copyright (C) 2006, 2007 Rob Buis <buis@kde.org>
 * Copyright (C) 2007, 2009 Apple Inc. All rights reserved.
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

#include "core/svg/SVGPathParser.h"

#include "core/svg/SVGPathConsumer.h"
#include "platform/transforms/AffineTransform.h"
#include "wtf/MathExtras.h"

namespace blink {

static FloatPoint reflectedPoint(const FloatPoint& reflectIn,
                                 const FloatPoint& pointToReflect) {
  return FloatPoint(2 * reflectIn.x() - pointToReflect.x(),
                    2 * reflectIn.y() - pointToReflect.y());
}

// Blend the points with a ratio (1/3):(2/3).
static FloatPoint blendPoints(const FloatPoint& p1, const FloatPoint& p2) {
  const float oneOverThree = 1 / 3.f;
  return FloatPoint((p1.x() + 2 * p2.x()) * oneOverThree,
                    (p1.y() + 2 * p2.y()) * oneOverThree);
}

static inline bool isCubicCommand(SVGPathSegType command) {
  return command == PathSegCurveToCubicAbs ||
         command == PathSegCurveToCubicRel ||
         command == PathSegCurveToCubicSmoothAbs ||
         command == PathSegCurveToCubicSmoothRel;
}

static inline bool isQuadraticCommand(SVGPathSegType command) {
  return command == PathSegCurveToQuadraticAbs ||
         command == PathSegCurveToQuadraticRel ||
         command == PathSegCurveToQuadraticSmoothAbs ||
         command == PathSegCurveToQuadraticSmoothRel;
}

void SVGPathNormalizer::emitSegment(const PathSegmentData& segment) {
  PathSegmentData normSeg = segment;

  // Convert relative points to absolute points.
  switch (segment.command) {
    case PathSegCurveToQuadraticRel:
      normSeg.point1 += m_currentPoint;
      normSeg.targetPoint += m_currentPoint;
      break;
    case PathSegCurveToCubicRel:
      normSeg.point1 += m_currentPoint;
    /* fall through */
    case PathSegCurveToCubicSmoothRel:
      normSeg.point2 += m_currentPoint;
    /* fall through */
    case PathSegMoveToRel:
    case PathSegLineToRel:
    case PathSegLineToHorizontalRel:
    case PathSegLineToVerticalRel:
    case PathSegCurveToQuadraticSmoothRel:
    case PathSegArcRel:
      normSeg.targetPoint += m_currentPoint;
      break;
    case PathSegLineToHorizontalAbs:
      normSeg.targetPoint.setY(m_currentPoint.y());
      break;
    case PathSegLineToVerticalAbs:
      normSeg.targetPoint.setX(m_currentPoint.x());
      break;
    case PathSegClosePath:
      // Reset m_currentPoint for the next path.
      normSeg.targetPoint = m_subPathPoint;
      break;
    default:
      break;
  }

  // Update command verb, handle smooth segments and convert quadratic curve
  // segments to cubics.
  switch (segment.command) {
    case PathSegMoveToRel:
    case PathSegMoveToAbs:
      m_subPathPoint = normSeg.targetPoint;
      normSeg.command = PathSegMoveToAbs;
      break;
    case PathSegLineToRel:
    case PathSegLineToAbs:
    case PathSegLineToHorizontalRel:
    case PathSegLineToHorizontalAbs:
    case PathSegLineToVerticalRel:
    case PathSegLineToVerticalAbs:
      normSeg.command = PathSegLineToAbs;
      break;
    case PathSegClosePath:
      normSeg.command = PathSegClosePath;
      break;
    case PathSegCurveToCubicSmoothRel:
    case PathSegCurveToCubicSmoothAbs:
      if (!isCubicCommand(m_lastCommand))
        normSeg.point1 = m_currentPoint;
      else
        normSeg.point1 = reflectedPoint(m_currentPoint, m_controlPoint);
    /* fall through */
    case PathSegCurveToCubicRel:
    case PathSegCurveToCubicAbs:
      m_controlPoint = normSeg.point2;
      normSeg.command = PathSegCurveToCubicAbs;
      break;
    case PathSegCurveToQuadraticSmoothRel:
    case PathSegCurveToQuadraticSmoothAbs:
      if (!isQuadraticCommand(m_lastCommand))
        normSeg.point1 = m_currentPoint;
      else
        normSeg.point1 = reflectedPoint(m_currentPoint, m_controlPoint);
    /* fall through */
    case PathSegCurveToQuadraticRel:
    case PathSegCurveToQuadraticAbs:
      // Save the unmodified control point.
      m_controlPoint = normSeg.point1;
      normSeg.point1 = blendPoints(m_currentPoint, m_controlPoint);
      normSeg.point2 = blendPoints(normSeg.targetPoint, m_controlPoint);
      normSeg.command = PathSegCurveToCubicAbs;
      break;
    case PathSegArcRel:
    case PathSegArcAbs:
      if (!decomposeArcToCubic(m_currentPoint, normSeg)) {
        // On failure, emit a line segment to the target point.
        normSeg.command = PathSegLineToAbs;
      } else {
        // decomposeArcToCubic() has already emitted the normalized
        // segments, so set command to PathSegArcAbs, to skip any further
        // emit.
        normSeg.command = PathSegArcAbs;
      }
      break;
    default:
      NOTREACHED();
  }

  if (normSeg.command != PathSegArcAbs)
    m_consumer->emitSegment(normSeg);

  m_currentPoint = normSeg.targetPoint;

  if (!isCubicCommand(segment.command) && !isQuadraticCommand(segment.command))
    m_controlPoint = m_currentPoint;

  m_lastCommand = segment.command;
}

// This works by converting the SVG arc to "simple" beziers.
// Partly adapted from Niko's code in kdelibs/kdecore/svgicons.
// See also SVG implementation notes:
// http://www.w3.org/TR/SVG/implnote.html#ArcConversionEndpointToCenter
bool SVGPathNormalizer::decomposeArcToCubic(const FloatPoint& currentPoint,
                                            const PathSegmentData& arcSegment) {
  // If rx = 0 or ry = 0 then this arc is treated as a straight line segment (a
  // "lineto") joining the endpoints.
  // http://www.w3.org/TR/SVG/implnote.html#ArcOutOfRangeParameters
  float rx = fabsf(arcSegment.arcRadii().x());
  float ry = fabsf(arcSegment.arcRadii().y());
  if (!rx || !ry)
    return false;

  // If the current point and target point for the arc are identical, it should
  // be treated as a zero length path. This ensures continuity in animations.
  if (arcSegment.targetPoint == currentPoint)
    return false;

  float angle = arcSegment.arcAngle();

  FloatSize midPointDistance = currentPoint - arcSegment.targetPoint;
  midPointDistance.scale(0.5f);

  AffineTransform pointTransform;
  pointTransform.rotate(-angle);

  FloatPoint transformedMidPoint = pointTransform.mapPoint(
      FloatPoint(midPointDistance.width(), midPointDistance.height()));
  float squareRx = rx * rx;
  float squareRy = ry * ry;
  float squareX = transformedMidPoint.x() * transformedMidPoint.x();
  float squareY = transformedMidPoint.y() * transformedMidPoint.y();

  // Check if the radii are big enough to draw the arc, scale radii if not.
  // http://www.w3.org/TR/SVG/implnote.html#ArcCorrectionOutOfRangeRadii
  float radiiScale = squareX / squareRx + squareY / squareRy;
  if (radiiScale > 1) {
    rx *= sqrtf(radiiScale);
    ry *= sqrtf(radiiScale);
  }

  pointTransform.makeIdentity();
  pointTransform.scale(1 / rx, 1 / ry);
  pointTransform.rotate(-angle);

  FloatPoint point1 = pointTransform.mapPoint(currentPoint);
  FloatPoint point2 = pointTransform.mapPoint(arcSegment.targetPoint);
  FloatSize delta = point2 - point1;

  float d = delta.width() * delta.width() + delta.height() * delta.height();
  float scaleFactorSquared = std::max(1 / d - 0.25f, 0.f);

  float scaleFactor = sqrtf(scaleFactorSquared);
  if (arcSegment.arcSweep == arcSegment.arcLarge)
    scaleFactor = -scaleFactor;

  delta.scale(scaleFactor);
  FloatPoint centerPoint = point1 + point2;
  centerPoint.scale(0.5f, 0.5f);
  centerPoint.move(-delta.height(), delta.width());

  float theta1 = FloatPoint(point1 - centerPoint).slopeAngleRadians();
  float theta2 = FloatPoint(point2 - centerPoint).slopeAngleRadians();

  float thetaArc = theta2 - theta1;
  if (thetaArc < 0 && arcSegment.arcSweep)
    thetaArc += twoPiFloat;
  else if (thetaArc > 0 && !arcSegment.arcSweep)
    thetaArc -= twoPiFloat;

  pointTransform.makeIdentity();
  pointTransform.rotate(angle);
  pointTransform.scale(rx, ry);

  // Some results of atan2 on some platform implementations are not exact
  // enough. So that we get more cubic curves than expected here. Adding 0.001f
  // reduces the count of sgements to the correct count.
  int segments = ceilf(fabsf(thetaArc / (piOverTwoFloat + 0.001f)));
  for (int i = 0; i < segments; ++i) {
    float startTheta = theta1 + i * thetaArc / segments;
    float endTheta = theta1 + (i + 1) * thetaArc / segments;

    float t = (8 / 6.f) * tanf(0.25f * (endTheta - startTheta));
    if (!std::isfinite(t))
      return false;
    float sinStartTheta = sinf(startTheta);
    float cosStartTheta = cosf(startTheta);
    float sinEndTheta = sinf(endTheta);
    float cosEndTheta = cosf(endTheta);

    point1 = FloatPoint(cosStartTheta - t * sinStartTheta,
                        sinStartTheta + t * cosStartTheta);
    point1.move(centerPoint.x(), centerPoint.y());
    FloatPoint targetPoint = FloatPoint(cosEndTheta, sinEndTheta);
    targetPoint.move(centerPoint.x(), centerPoint.y());
    point2 = targetPoint;
    point2.move(t * sinEndTheta, -t * cosEndTheta);

    PathSegmentData cubicSegment;
    cubicSegment.command = PathSegCurveToCubicAbs;
    cubicSegment.point1 = pointTransform.mapPoint(point1);
    cubicSegment.point2 = pointTransform.mapPoint(point2);
    cubicSegment.targetPoint = pointTransform.mapPoint(targetPoint);

    m_consumer->emitSegment(cubicSegment);
  }
  return true;
}

}  // namespace blink
