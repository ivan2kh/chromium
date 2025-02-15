// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/content_accelerators/accelerator_util.h"

#include "build/build_config.h"
#include "third_party/WebKit/public/platform/WebInputEvent.h"
#include "ui/events/event.h"
#include "ui/events/event_constants.h"

namespace ui {

namespace {

int GetModifiersFromNativeWebKeyboardEvent(
    const content::NativeWebKeyboardEvent& event) {
  int modifiers = ui::EF_NONE;
  if (event.modifiers() & content::NativeWebKeyboardEvent::ShiftKey)
    modifiers |= ui::EF_SHIFT_DOWN;
  if (event.modifiers() & content::NativeWebKeyboardEvent::ControlKey)
    modifiers |= ui::EF_CONTROL_DOWN;
  if (event.modifiers() & content::NativeWebKeyboardEvent::AltKey)
    modifiers |= ui::EF_ALT_DOWN;
#if defined(OS_MACOSX) || defined(OS_CHROMEOS)
  if (event.modifiers() & content::NativeWebKeyboardEvent::MetaKey)
    modifiers |= ui::EF_COMMAND_DOWN;
#endif
#if defined(USE_AURA)
  if (event.os_event && static_cast<ui::KeyEvent*>(event.os_event)->is_repeat())
    modifiers |= ui::EF_IS_REPEAT;
#endif
  return modifiers;
}

}  // namespace

ui::Accelerator GetAcceleratorFromNativeWebKeyboardEvent(
    const content::NativeWebKeyboardEvent& event) {
  ui::Accelerator accelerator(
      static_cast<ui::KeyboardCode>(event.windowsKeyCode),
      GetModifiersFromNativeWebKeyboardEvent(event));
  if (event.type() == blink::WebInputEvent::KeyUp)
    accelerator.set_key_state(Accelerator::KeyState::RELEASED);
  return accelerator;
}

}  // namespace ui
