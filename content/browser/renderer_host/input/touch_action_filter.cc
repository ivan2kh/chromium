// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/renderer_host/input/touch_action_filter.h"

#include <math.h>

#include "base/logging.h"
#include "third_party/WebKit/public/platform/WebGestureEvent.h"

using blink::WebInputEvent;
using blink::WebGestureEvent;

namespace content {
namespace {

// Actions on an axis are disallowed if the perpendicular axis has a filter set
// and no filter is set for the queried axis.
bool IsYAxisActionDisallowed(TouchAction action) {
  return (action & TOUCH_ACTION_PAN_X) && !(action & TOUCH_ACTION_PAN_Y);
}

bool IsXAxisActionDisallowed(TouchAction action) {
  return (action & TOUCH_ACTION_PAN_Y) && !(action & TOUCH_ACTION_PAN_X);
}

}  // namespace

TouchActionFilter::TouchActionFilter()
    : suppress_manipulation_events_(false),
      drop_current_tap_ending_event_(false),
      allow_current_double_tap_event_(true),
      allowed_touch_action_(TOUCH_ACTION_AUTO) {}

bool TouchActionFilter::FilterGestureEvent(WebGestureEvent* gesture_event) {
  if (gesture_event->sourceDevice != blink::WebGestureDeviceTouchscreen)
    return false;

  // Filter for allowable touch actions first (eg. before the TouchEventQueue
  // can decide to send a touch cancel event).
  switch (gesture_event->type()) {
    case WebInputEvent::GestureScrollBegin:
      DCHECK(!suppress_manipulation_events_);
      suppress_manipulation_events_ =
          ShouldSuppressManipulation(*gesture_event);
      return suppress_manipulation_events_;

    case WebInputEvent::GestureScrollUpdate:
      if (suppress_manipulation_events_)
        return true;

      // Scrolls restricted to a specific axis shouldn't permit movement
      // in the perpendicular axis.
      //
      // Note the direction suppression with pinch-zoom here, which matches
      // Edge: a "touch-action: pan-y pinch-zoom" region allows vertical
      // two-finger scrolling but a "touch-action: pan-x pinch-zoom" region
      // doesn't.
      // TODO(mustaq): Add it to spec?
      if (IsYAxisActionDisallowed(allowed_touch_action_)) {
        gesture_event->data.scrollUpdate.deltaY = 0;
        gesture_event->data.scrollUpdate.velocityY = 0;
      } else if (IsXAxisActionDisallowed(allowed_touch_action_)) {
        gesture_event->data.scrollUpdate.deltaX = 0;
        gesture_event->data.scrollUpdate.velocityX = 0;
      }
      break;

    case WebInputEvent::GestureFlingStart:
      // Touchscreen flings should always have non-zero velocity.
      DCHECK(gesture_event->data.flingStart.velocityX ||
             gesture_event->data.flingStart.velocityY);
      if (!suppress_manipulation_events_) {
        // Flings restricted to a specific axis shouldn't permit velocity
        // in the perpendicular axis.
        if (IsYAxisActionDisallowed(allowed_touch_action_))
          gesture_event->data.flingStart.velocityY = 0;
        else if (IsXAxisActionDisallowed(allowed_touch_action_))
          gesture_event->data.flingStart.velocityX = 0;
        // As the renderer expects a scroll-ending event, but does not expect a
        // zero-velocity fling, convert the now zero-velocity fling accordingly.
        if (!gesture_event->data.flingStart.velocityX &&
            !gesture_event->data.flingStart.velocityY) {
          gesture_event->setType(WebInputEvent::GestureScrollEnd);
        }
      }
      return FilterManipulationEventAndResetState();

    case WebInputEvent::GestureScrollEnd:
      return FilterManipulationEventAndResetState();

    case WebInputEvent::GesturePinchBegin:
    case WebInputEvent::GesturePinchUpdate:
    case WebInputEvent::GesturePinchEnd:
      return suppress_manipulation_events_;

    // The double tap gesture is a tap ending event. If a double tap gesture is
    // filtered out, replace it with a tap event.
    case WebInputEvent::GestureDoubleTap:
      DCHECK_EQ(1, gesture_event->data.tap.tapCount);
      if (!allow_current_double_tap_event_)
        gesture_event->setType(WebInputEvent::GestureTap);
      allow_current_double_tap_event_ = true;
      break;

    // If double tap is disabled, there's no reason for the tap delay.
    case WebInputEvent::GestureTapUnconfirmed:
      DCHECK_EQ(1, gesture_event->data.tap.tapCount);
      allow_current_double_tap_event_ =
          (allowed_touch_action_ & TOUCH_ACTION_DOUBLE_TAP_ZOOM) != 0;
      if (!allow_current_double_tap_event_) {
        gesture_event->setType(WebInputEvent::GestureTap);
        drop_current_tap_ending_event_ = true;
      }
      break;

    case WebInputEvent::GestureTap:
      allow_current_double_tap_event_ =
          (allowed_touch_action_ & TOUCH_ACTION_DOUBLE_TAP_ZOOM) != 0;
      // Fall through.

    case WebInputEvent::GestureTapCancel:
      if (drop_current_tap_ending_event_) {
        drop_current_tap_ending_event_ = false;
        return true;
      }
      break;

    case WebInputEvent::GestureTapDown:
      DCHECK(!drop_current_tap_ending_event_);
      break;

    default:
      // Gesture events unrelated to touch actions (panning/zooming) are left
      // alone.
      break;
  }

  return false;
}

bool TouchActionFilter::FilterManipulationEventAndResetState() {
  if (suppress_manipulation_events_) {
    suppress_manipulation_events_ = false;
    return true;
  }
  return false;
}

void TouchActionFilter::OnSetTouchAction(TouchAction touch_action) {
  // For multiple fingers, we take the intersection of the touch actions for
  // all fingers that have gone down during this action.  In the majority of
  // real-world scenarios the touch action for all fingers will be the same.
  // This is left as implementation-defined in the pointer events
  // specification because of the relationship to gestures (which are off
  // limits for the spec).  I believe the following are desirable properties
  // of this choice:
  // 1. Not sensitive to finger touch order.  Behavior of putting two fingers
  //    down "at once" will be deterministic.
  // 2. Only subtractive - eg. can't trigger scrolling on a element that
  //    otherwise has scrolling disabling by the addition of a finger.
  allowed_touch_action_ &= touch_action;
}

void TouchActionFilter::ResetTouchAction() {
  // Note that resetting the action mid-sequence is tolerated. Gestures that had
  // their begin event(s) suppressed will be suppressed until the next sequence.
  allowed_touch_action_ = TOUCH_ACTION_AUTO;
}

bool TouchActionFilter::ShouldSuppressManipulation(
    const blink::WebGestureEvent& gesture_event) {
  DCHECK_EQ(gesture_event.type(), WebInputEvent::GestureScrollBegin);

  if (gesture_event.data.scrollBegin.pointerCount >= 2) {
    // Any GestureScrollBegin with more than one fingers is like a pinch-zoom
    // for touch-actions, see crbug.com/632525. Therefore, we switch to
    // blocked-manipulation mode iff pinch-zoom is disallowed.
    return (allowed_touch_action_ & TOUCH_ACTION_PINCH_ZOOM) == 0;
  }

  const float& deltaXHint = gesture_event.data.scrollBegin.deltaXHint;
  const float& deltaYHint = gesture_event.data.scrollBegin.deltaYHint;

  if (deltaXHint == 0.0 && deltaYHint == 0.0)
    return false;

  const float absDeltaXHint = fabs(deltaXHint);
  const float absDeltaYHint = fabs(deltaYHint);

  TouchAction minimal_conforming_touch_action = TOUCH_ACTION_NONE;
  if (absDeltaXHint >= absDeltaYHint) {
    if (deltaXHint > 0)
      minimal_conforming_touch_action |= TOUCH_ACTION_PAN_LEFT;
    else if (deltaXHint < 0)
      minimal_conforming_touch_action |= TOUCH_ACTION_PAN_RIGHT;
  }
  if (absDeltaYHint >= absDeltaXHint) {
    if (deltaYHint > 0)
      minimal_conforming_touch_action |= TOUCH_ACTION_PAN_UP;
    else if (deltaYHint < 0)
      minimal_conforming_touch_action |= TOUCH_ACTION_PAN_DOWN;
  }
  DCHECK(minimal_conforming_touch_action != TOUCH_ACTION_NONE);

  return (allowed_touch_action_ & minimal_conforming_touch_action) == 0;
}

}  // namespace content
