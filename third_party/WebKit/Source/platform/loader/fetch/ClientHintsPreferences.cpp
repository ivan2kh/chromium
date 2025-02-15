// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "platform/loader/fetch/ClientHintsPreferences.h"

#include "platform/RuntimeEnabledFeatures.h"
#include "platform/network/HTTPParsers.h"

namespace blink {

ClientHintsPreferences::ClientHintsPreferences()
    : m_shouldSendDPR(false),
      m_shouldSendResourceWidth(false),
      m_shouldSendViewportWidth(false) {}

void ClientHintsPreferences::updateFrom(
    const ClientHintsPreferences& preferences) {
  m_shouldSendDPR = preferences.m_shouldSendDPR;
  m_shouldSendResourceWidth = preferences.m_shouldSendResourceWidth;
  m_shouldSendViewportWidth = preferences.m_shouldSendViewportWidth;
}

void ClientHintsPreferences::updateFromAcceptClientHintsHeader(
    const String& headerValue,
    Context* context) {
  if (!RuntimeEnabledFeatures::clientHintsEnabled() || headerValue.isEmpty())
    return;

  CommaDelimitedHeaderSet acceptClientHintsHeader;
  parseCommaDelimitedHeader(headerValue, acceptClientHintsHeader);
  if (acceptClientHintsHeader.contains("dpr")) {
    if (context)
      context->countClientHintsDPR();
    m_shouldSendDPR = true;
  }

  if (acceptClientHintsHeader.contains("width")) {
    if (context)
      context->countClientHintsResourceWidth();
    m_shouldSendResourceWidth = true;
  }

  if (acceptClientHintsHeader.contains("viewport-width")) {
    if (context)
      context->countClientHintsViewportWidth();
    m_shouldSendViewportWidth = true;
  }
}

}  // namespace blink
