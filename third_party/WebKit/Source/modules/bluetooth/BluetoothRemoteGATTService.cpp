// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "modules/bluetooth/BluetoothRemoteGATTService.h"

#include "bindings/core/v8/ScriptPromise.h"
#include "bindings/core/v8/ScriptPromiseResolver.h"
#include "core/dom/DOMException.h"
#include "core/dom/ExceptionCode.h"
#include "core/inspector/ConsoleMessage.h"
#include "modules/bluetooth/Bluetooth.h"
#include "modules/bluetooth/BluetoothError.h"
#include "modules/bluetooth/BluetoothRemoteGATTCharacteristic.h"
#include "modules/bluetooth/BluetoothUUID.h"
#include "wtf/PtrUtil.h"
#include <utility>

namespace blink {

BluetoothRemoteGATTService::BluetoothRemoteGATTService(
    mojom::blink::WebBluetoothRemoteGATTServicePtr service,
    bool isPrimary,
    const String& deviceInstanceId,
    BluetoothDevice* device)
    : m_service(std::move(service)),
      m_isPrimary(isPrimary),
      m_deviceInstanceId(deviceInstanceId),
      m_device(device) {}

DEFINE_TRACE(BluetoothRemoteGATTService) {
  visitor->trace(m_device);
}

// Callback that allows us to resolve the promise with a single characteristic
// or with a vector owning the characteristics.
void BluetoothRemoteGATTService::GetCharacteristicsCallback(
    const String& serviceInstanceId,
    const String& requestedCharacteristicUUID,
    mojom::blink::WebBluetoothGATTQueryQuantity quantity,
    ScriptPromiseResolver* resolver,
    mojom::blink::WebBluetoothResult result,
    Optional<Vector<mojom::blink::WebBluetoothRemoteGATTCharacteristicPtr>>
        characteristics) {
  if (!resolver->getExecutionContext() ||
      resolver->getExecutionContext()->isContextDestroyed())
    return;

  // If the device is disconnected, reject.
  if (!m_device->gatt()->RemoveFromActiveAlgorithms(resolver)) {
    resolver->reject(BluetoothError::CreateNotConnectedException(
        BluetoothOperation::CharacteristicsRetrieval));
    return;
  }

  if (result == mojom::blink::WebBluetoothResult::SUCCESS) {
    DCHECK(characteristics);

    if (quantity == mojom::blink::WebBluetoothGATTQueryQuantity::SINGLE) {
      DCHECK_EQ(1u, characteristics->size());
      resolver->resolve(m_device->GetOrCreateRemoteGATTCharacteristic(
          resolver->getExecutionContext(),
          std::move(characteristics.value()[0]), this));
      return;
    }

    HeapVector<Member<BluetoothRemoteGATTCharacteristic>> gattCharacteristics;
    gattCharacteristics.reserveInitialCapacity(characteristics->size());
    for (auto& characteristic : characteristics.value()) {
      gattCharacteristics.push_back(
          m_device->GetOrCreateRemoteGATTCharacteristic(
              resolver->getExecutionContext(), std::move(characteristic),
              this));
    }
    resolver->resolve(gattCharacteristics);
  } else {
    if (result == mojom::blink::WebBluetoothResult::CHARACTERISTIC_NOT_FOUND) {
      resolver->reject(BluetoothError::CreateDOMException(
          BluetoothErrorCode::CharacteristicNotFound,
          "No Characteristics matching UUID " + requestedCharacteristicUUID +
              " found in Service with UUID " + uuid() + "."));
    } else {
      resolver->reject(BluetoothError::CreateDOMException(result));
    }
  }
}

ScriptPromise BluetoothRemoteGATTService::getCharacteristic(
    ScriptState* scriptState,
    const StringOrUnsignedLong& characteristic,
    ExceptionState& exceptionState) {
  String characteristicUUID =
      BluetoothUUID::getCharacteristic(characteristic, exceptionState);
  if (exceptionState.hadException())
    return exceptionState.reject(scriptState);

  return GetCharacteristicsImpl(
      scriptState, mojom::blink::WebBluetoothGATTQueryQuantity::SINGLE,
      characteristicUUID);
}

ScriptPromise BluetoothRemoteGATTService::getCharacteristics(
    ScriptState* scriptState,
    const StringOrUnsignedLong& characteristic,
    ExceptionState& exceptionState) {
  String characteristicUUID =
      BluetoothUUID::getCharacteristic(characteristic, exceptionState);
  if (exceptionState.hadException())
    return exceptionState.reject(scriptState);

  return GetCharacteristicsImpl(
      scriptState, mojom::blink::WebBluetoothGATTQueryQuantity::MULTIPLE,
      characteristicUUID);
}

ScriptPromise BluetoothRemoteGATTService::getCharacteristics(
    ScriptState* scriptState,
    ExceptionState&) {
  return GetCharacteristicsImpl(
      scriptState, mojom::blink::WebBluetoothGATTQueryQuantity::MULTIPLE);
}

ScriptPromise BluetoothRemoteGATTService::GetCharacteristicsImpl(
    ScriptState* scriptState,
    mojom::blink::WebBluetoothGATTQueryQuantity quantity,
    const String& characteristicsUUID) {
  if (!m_device->gatt()->connected()) {
    return ScriptPromise::rejectWithDOMException(
        scriptState, BluetoothError::CreateNotConnectedException(
                         BluetoothOperation::CharacteristicsRetrieval));
  }

  if (!m_device->IsValidService(m_service->instance_id)) {
    return ScriptPromise::rejectWithDOMException(
        scriptState, BluetoothError::CreateDOMException(
                         BluetoothErrorCode::InvalidService,
                         "Service with UUID " + m_service->uuid +
                             " is no longer valid. Remember to retrieve "
                             "the service again after reconnecting."));
  }

  ScriptPromiseResolver* resolver = ScriptPromiseResolver::create(scriptState);
  ScriptPromise promise = resolver->promise();
  m_device->gatt()->AddToActiveAlgorithms(resolver);

  mojom::blink::WebBluetoothService* service = m_device->bluetooth()->Service();
  service->RemoteServiceGetCharacteristics(
      m_service->instance_id, quantity, characteristicsUUID,
      convertToBaseCallback(
          WTF::bind(&BluetoothRemoteGATTService::GetCharacteristicsCallback,
                    wrapPersistent(this), m_service->instance_id,
                    characteristicsUUID, quantity, wrapPersistent(resolver))));

  return promise;
}

}  // namespace blink
