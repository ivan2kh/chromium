// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This file has been auto-generated by code_generator_v8.py.
// DO NOT MODIFY!

// This file has been generated from the Jinja2 template in
// third_party/WebKit/Source/bindings/templates/union_container.cpp.tmpl

// clang-format off
#include "DoubleOrLongOrBooleanSequence.h"

#include "bindings/core/v8/IDLTypes.h"
#include "bindings/core/v8/LongOrBoolean.h"
#include "bindings/core/v8/NativeValueTraitsImpl.h"
#include "bindings/core/v8/ToV8.h"

namespace blink {

DoubleOrLongOrBooleanSequence::DoubleOrLongOrBooleanSequence() : m_type(SpecificTypeNone) {}

double DoubleOrLongOrBooleanSequence::getAsDouble() const {
  DCHECK(isDouble());
  return m_double;
}

void DoubleOrLongOrBooleanSequence::setDouble(double value) {
  DCHECK(isNull());
  m_double = value;
  m_type = SpecificTypeDouble;
}

DoubleOrLongOrBooleanSequence DoubleOrLongOrBooleanSequence::fromDouble(double value) {
  DoubleOrLongOrBooleanSequence container;
  container.setDouble(value);
  return container;
}

const HeapVector<LongOrBoolean>& DoubleOrLongOrBooleanSequence::getAsLongOrBooleanSequence() const {
  DCHECK(isLongOrBooleanSequence());
  return m_longOrBooleanSequence;
}

void DoubleOrLongOrBooleanSequence::setLongOrBooleanSequence(const HeapVector<LongOrBoolean>& value) {
  DCHECK(isNull());
  m_longOrBooleanSequence = value;
  m_type = SpecificTypeLongOrBooleanSequence;
}

DoubleOrLongOrBooleanSequence DoubleOrLongOrBooleanSequence::fromLongOrBooleanSequence(const HeapVector<LongOrBoolean>& value) {
  DoubleOrLongOrBooleanSequence container;
  container.setLongOrBooleanSequence(value);
  return container;
}

DoubleOrLongOrBooleanSequence::DoubleOrLongOrBooleanSequence(const DoubleOrLongOrBooleanSequence&) = default;
DoubleOrLongOrBooleanSequence::~DoubleOrLongOrBooleanSequence() = default;
DoubleOrLongOrBooleanSequence& DoubleOrLongOrBooleanSequence::operator=(const DoubleOrLongOrBooleanSequence&) = default;

DEFINE_TRACE(DoubleOrLongOrBooleanSequence) {
  visitor->trace(m_longOrBooleanSequence);
}

void V8DoubleOrLongOrBooleanSequence::toImpl(v8::Isolate* isolate, v8::Local<v8::Value> v8Value, DoubleOrLongOrBooleanSequence& impl, UnionTypeConversionMode conversionMode, ExceptionState& exceptionState) {
  if (v8Value.IsEmpty())
    return;

  if (conversionMode == UnionTypeConversionMode::Nullable && isUndefinedOrNull(v8Value))
    return;

  if (v8Value->IsArray()) {
    HeapVector<LongOrBoolean> cppValue = toImplArray<HeapVector<LongOrBoolean>>(v8Value, 0, isolate, exceptionState);
    if (exceptionState.hadException())
      return;
    impl.setLongOrBooleanSequence(cppValue);
    return;
  }

  if (v8Value->IsNumber()) {
    double cppValue = NativeValueTraits<IDLDouble>::nativeValue(isolate, v8Value, exceptionState);
    if (exceptionState.hadException())
      return;
    impl.setDouble(cppValue);
    return;
  }

  {
    double cppValue = NativeValueTraits<IDLDouble>::nativeValue(isolate, v8Value, exceptionState);
    if (exceptionState.hadException())
      return;
    impl.setDouble(cppValue);
    return;
  }
}

v8::Local<v8::Value> ToV8(const DoubleOrLongOrBooleanSequence& impl, v8::Local<v8::Object> creationContext, v8::Isolate* isolate) {
  switch (impl.m_type) {
    case DoubleOrLongOrBooleanSequence::SpecificTypeNone:
      return v8::Null(isolate);
    case DoubleOrLongOrBooleanSequence::SpecificTypeDouble:
      return v8::Number::New(isolate, impl.getAsDouble());
    case DoubleOrLongOrBooleanSequence::SpecificTypeLongOrBooleanSequence:
      return ToV8(impl.getAsLongOrBooleanSequence(), creationContext, isolate);
    default:
      NOTREACHED();
  }
  return v8::Local<v8::Value>();
}

DoubleOrLongOrBooleanSequence NativeValueTraits<DoubleOrLongOrBooleanSequence>::nativeValue(v8::Isolate* isolate, v8::Local<v8::Value> value, ExceptionState& exceptionState) {
  DoubleOrLongOrBooleanSequence impl;
  V8DoubleOrLongOrBooleanSequence::toImpl(isolate, value, impl, UnionTypeConversionMode::NotNullable, exceptionState);
  return impl;
}

}  // namespace blink
