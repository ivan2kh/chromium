// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "modules/netinfo/WorkerNavigatorNetworkInformation.h"

#include "bindings/core/v8/ScriptState.h"
#include "core/workers/WorkerNavigator.h"
#include "modules/netinfo/NetworkInformation.h"

namespace blink {

WorkerNavigatorNetworkInformation::WorkerNavigatorNetworkInformation(
    WorkerNavigator& navigator,
    ExecutionContext* context)
    : Supplement<WorkerNavigator>(navigator) {}

WorkerNavigatorNetworkInformation& WorkerNavigatorNetworkInformation::from(
    WorkerNavigator& navigator,
    ExecutionContext* context) {
  WorkerNavigatorNetworkInformation* supplement =
      toWorkerNavigatorNetworkInformation(navigator, context);
  if (!supplement) {
    supplement = new WorkerNavigatorNetworkInformation(navigator, context);
    provideTo(navigator, supplementName(), supplement);
  }
  return *supplement;
}

WorkerNavigatorNetworkInformation*
WorkerNavigatorNetworkInformation::toWorkerNavigatorNetworkInformation(
    WorkerNavigator& navigator,
    ExecutionContext* context) {
  return static_cast<WorkerNavigatorNetworkInformation*>(
      Supplement<WorkerNavigator>::from(navigator, supplementName()));
}

const char* WorkerNavigatorNetworkInformation::supplementName() {
  return "WorkerNavigatorNetworkInformation";
}

NetworkInformation* WorkerNavigatorNetworkInformation::connection(
    ScriptState* scriptState,
    WorkerNavigator& navigator) {
  ExecutionContext* context = scriptState->getExecutionContext();
  return WorkerNavigatorNetworkInformation::from(navigator, context)
      .connection(context);
}

DEFINE_TRACE(WorkerNavigatorNetworkInformation) {
  visitor->trace(m_connection);
  Supplement<WorkerNavigator>::trace(visitor);
}

NetworkInformation* WorkerNavigatorNetworkInformation::connection(
    ExecutionContext* context) {
  ASSERT(context);
  if (!m_connection)
    m_connection = NetworkInformation::create(context);
  return m_connection.get();
}

}  // namespace blink
