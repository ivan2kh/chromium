// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DEVICE_VR_VR_DEVICE_H
#define DEVICE_VR_VR_DEVICE_H

#include "base/callback.h"
#include "base/macros.h"
#include "device/vr/vr_export.h"
#include "device/vr/vr_service.mojom.h"

namespace device {

class VRDisplayImpl;

const unsigned int VR_DEVICE_LAST_ID = 0xFFFFFFFF;

class DEVICE_VR_EXPORT VRDevice {
 public:
  VRDevice();
  virtual ~VRDevice();

  unsigned int id() const { return id_; }

  virtual void GetVRDevice(
      const base::Callback<void(mojom::VRDisplayInfoPtr)>& callback) = 0;
  virtual void ResetPose() = 0;

  virtual void RequestPresent(mojom::VRSubmitFrameClientPtr submit_client,
                              const base::Callback<void(bool)>& callback) = 0;
  virtual void SetSecureOrigin(bool secure_origin) = 0;
  virtual void ExitPresent() = 0;
  virtual void SubmitFrame(int16_t frame_index,
                           const gpu::MailboxHolder& mailbox) = 0;
  virtual void UpdateLayerBounds(int16_t frame_index,
                                 mojom::VRLayerBoundsPtr left_bounds,
                                 mojom::VRLayerBoundsPtr right_bounds,
                                 int16_t source_width,
                                 int16_t source_height) = 0;
  virtual void GetVRVSyncProvider(mojom::VRVSyncProviderRequest request) = 0;

  virtual void AddDisplay(VRDisplayImpl* display);
  virtual void RemoveDisplay(VRDisplayImpl* display);

  virtual bool IsAccessAllowed(VRDisplayImpl* display);
  virtual bool CheckPresentingDisplay(VRDisplayImpl* display);

  virtual void OnChanged();
  virtual void OnExitPresent();
  virtual void OnBlur();
  virtual void OnFocus();
  virtual void OnActivate(mojom::VRDisplayEventReason reason);
  virtual void OnDeactivate(mojom::VRDisplayEventReason reason);

 protected:
  friend class VRDisplayImpl;
  friend class VRDisplayImplTest;

  void SetPresentingDisplay(VRDisplayImpl* display);

 private:
  void OnVRDisplayInfoCreated(mojom::VRDisplayInfoPtr vr_device_info);

  std::set<VRDisplayImpl*> displays_;

  VRDisplayImpl* presenting_display_;

  unsigned int id_;

  static unsigned int next_id_;

  base::WeakPtrFactory<VRDevice> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(VRDevice);
};

}  // namespace device

#endif  // DEVICE_VR_VR_DEVICE_H
