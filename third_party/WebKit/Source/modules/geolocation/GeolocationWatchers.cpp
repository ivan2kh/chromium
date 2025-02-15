// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "modules/geolocation/GeolocationWatchers.h"

#include "modules/geolocation/GeoNotifier.h"
#include "wtf/Assertions.h"

namespace blink {

DEFINE_TRACE(GeolocationWatchers) {
  visitor->trace(m_idToNotifierMap);
  visitor->trace(m_notifierToIdMap);
}

bool GeolocationWatchers::add(int id, GeoNotifier* notifier) {
  DCHECK_GT(id, 0);
  if (!m_idToNotifierMap.insert(id, notifier).isNewEntry)
    return false;
  m_notifierToIdMap.set(notifier, id);
  return true;
}

GeoNotifier* GeolocationWatchers::find(int id) {
  DCHECK_GT(id, 0);
  IdToNotifierMap::iterator iter = m_idToNotifierMap.find(id);
  if (iter == m_idToNotifierMap.end())
    return 0;
  return iter->value;
}

void GeolocationWatchers::remove(int id) {
  DCHECK_GT(id, 0);
  IdToNotifierMap::iterator iter = m_idToNotifierMap.find(id);
  if (iter == m_idToNotifierMap.end())
    return;
  m_notifierToIdMap.erase(iter->value);
  m_idToNotifierMap.remove(iter);
}

void GeolocationWatchers::remove(GeoNotifier* notifier) {
  NotifierToIdMap::iterator iter = m_notifierToIdMap.find(notifier);
  if (iter == m_notifierToIdMap.end())
    return;
  m_idToNotifierMap.erase(iter->value);
  m_notifierToIdMap.remove(iter);
}

bool GeolocationWatchers::contains(GeoNotifier* notifier) const {
  return m_notifierToIdMap.contains(notifier);
}

void GeolocationWatchers::clear() {
  m_idToNotifierMap.clear();
  m_notifierToIdMap.clear();
}

bool GeolocationWatchers::isEmpty() const {
  return m_idToNotifierMap.isEmpty();
}

void GeolocationWatchers::getNotifiersVector(
    HeapVector<Member<GeoNotifier>>& copy) const {
  copyValuesToVector(m_idToNotifierMap, copy);
}

}  // namespace blink
