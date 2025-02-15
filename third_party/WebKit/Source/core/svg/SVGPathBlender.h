/*
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

#ifndef SVGPathBlender_h
#define SVGPathBlender_h

#include "platform/heap/Handle.h"
#include "wtf/Noncopyable.h"

namespace blink {

class SVGPathConsumer;
class SVGPathByteStreamSource;

class SVGPathBlender final {
  WTF_MAKE_NONCOPYABLE(SVGPathBlender);
  STACK_ALLOCATED();

 public:
  SVGPathBlender(SVGPathByteStreamSource* fromSource,
                 SVGPathByteStreamSource* toSource,
                 SVGPathConsumer*);

  bool addAnimatedPath(unsigned repeatCount);
  bool blendAnimatedPath(float);

 private:
  class BlendState;
  bool blendAnimatedPath(BlendState&);

  SVGPathByteStreamSource* m_fromSource;
  SVGPathByteStreamSource* m_toSource;
  SVGPathConsumer* m_consumer;
};

}  // namespace blink

#endif  // SVGPathBlender_h
