// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_MIDI_DYNAMICALLY_INITIALIZED_MIDI_MANAGER_WIN_H_
#define MEDIA_MIDI_DYNAMICALLY_INITIALIZED_MIDI_MANAGER_WIN_H_

#include <memory>
#include <vector>

#include "base/callback_forward.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/system_monitor/system_monitor.h"
#include "media/midi/midi_manager.h"

namespace base {
class SingleThreadTaskRunner;
class TimeDelta;
}  // namespace base

namespace midi {

// New backend for legacy Windows that support dynamic instantiation.
class DynamicallyInitializedMidiManagerWin final
    : public MidiManager,
      public base::SystemMonitor::DevicesChangedObserver {
 public:
  class PortManager;

  explicit DynamicallyInitializedMidiManagerWin(MidiService* service);
  ~DynamicallyInitializedMidiManagerWin() override;

  // Returns PortManager that implements interfaces to help implementation.
  // This hides Windows specific structures, i.e. HMIDIIN in the header.
  PortManager* port_manager() { return port_manager_.get(); }

  // MidiManager overrides:
  void StartInitialization() override;
  void Finalize() override;
  void DispatchSendMidiData(MidiManagerClient* client,
                            uint32_t port_index,
                            const std::vector<uint8_t>& data,
                            double timestamp) override;

  // base::SystemMonitor::DevicesChangedObserver overrides:
  void OnDevicesChanged(base::SystemMonitor::DeviceType device_type) override;

 private:
  class InPort;
  class OutPort;

  // Handles MIDI inport event posted from a thread system provides.
  void ReceiveMidiData(uint32_t index,
                       const std::vector<uint8_t>& data,
                       base::TimeTicks time);

  // Posts a task to TaskRunner, and ensures that the instance keeps alive while
  // the task is running.
  void PostTask(const base::Closure&);
  void PostDelayedTask(const base::Closure&, base::TimeDelta delay);

  // Posts a reply task to the I/O thread that hosts MidiManager instance, runs
  // it safely, and ensures that the instance keeps alive while the task is
  // running.
  void PostReplyTask(const base::Closure&);

  // Initializes instance asynchronously on TaskRunner.
  void InitializeOnTaskRunner();

  // Updates device lists on TaskRunner.
  // Returns true if device lists were changed.
  void UpdateDeviceListOnTaskRunner();

  // Reflect active port list to a device list.
  template <typename T>
  void ReflectActiveDeviceList(DynamicallyInitializedMidiManagerWin* manager,
                               std::vector<T>* known_ports,
                               std::vector<T>* active_ports);

  // Sends MIDI data on TaskRunner.
  void SendOnTaskRunner(MidiManagerClient* client,
                        uint32_t port_index,
                        const std::vector<uint8_t>& data);

  // Holds an unique instance ID.
  const int instance_id_;

  // Keeps a TaskRunner for the I/O thread.
  scoped_refptr<base::SingleThreadTaskRunner> thread_runner_;

  // Manages platform dependent implementation for port managegent. Should be
  // accessed with the task lock.
  std::unique_ptr<PortManager> port_manager_;

  DISALLOW_COPY_AND_ASSIGN(DynamicallyInitializedMidiManagerWin);
};

}  // namespace midi

#endif  // MEDIA_MIDI_DYNAMICALLY_INITIALIZED_MIDI_MANAGER_WIN_H_
