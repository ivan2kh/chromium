// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/android/event_forwarder.h"

#include "jni/EventForwarder_jni.h"
#include "ui/android/view_android.h"
#include "ui/android/view_client.h"
#include "ui/events/android/motion_event_android.h"

namespace ui {

using base::android::JavaParamRef;
using base::android::ScopedJavaLocalRef;

EventForwarder::EventForwarder(ViewAndroid* view) : view_(view) {}

EventForwarder::~EventForwarder() {
  if (!java_obj_.is_null()) {
    Java_EventForwarder_destroy(base::android::AttachCurrentThread(),
                                java_obj_);
    java_obj_.Reset();
  }
}

ScopedJavaLocalRef<jobject> EventForwarder::GetJavaObject() {
  if (java_obj_.is_null()) {
    java_obj_.Reset(
        Java_EventForwarder_create(base::android::AttachCurrentThread(),
                                   reinterpret_cast<intptr_t>(this)));
  }
  return ScopedJavaLocalRef<jobject>(java_obj_);
}

jboolean EventForwarder::OnTouchEvent(JNIEnv* env,
                                      const JavaParamRef<jobject>& obj,
                                      const JavaParamRef<jobject>& motion_event,
                                      jlong time_ms,
                                      jint android_action,
                                      jint pointer_count,
                                      jint history_size,
                                      jint action_index,
                                      jfloat pos_x_0,
                                      jfloat pos_y_0,
                                      jfloat pos_x_1,
                                      jfloat pos_y_1,
                                      jint pointer_id_0,
                                      jint pointer_id_1,
                                      jfloat touch_major_0,
                                      jfloat touch_major_1,
                                      jfloat touch_minor_0,
                                      jfloat touch_minor_1,
                                      jfloat orientation_0,
                                      jfloat orientation_1,
                                      jfloat tilt_0,
                                      jfloat tilt_1,
                                      jfloat raw_pos_x,
                                      jfloat raw_pos_y,
                                      jint android_tool_type_0,
                                      jint android_tool_type_1,
                                      jint android_button_state,
                                      jint android_meta_state,
                                      jboolean is_touch_handle_event) {
  ui::MotionEventAndroid::Pointer pointer0(
      pointer_id_0, pos_x_0, pos_y_0, touch_major_0, touch_minor_0,
      orientation_0, tilt_0, android_tool_type_0);
  ui::MotionEventAndroid::Pointer pointer1(
      pointer_id_1, pos_x_1, pos_y_1, touch_major_1, touch_minor_1,
      orientation_1, tilt_1, android_tool_type_1);
  ui::MotionEventAndroid event(
      1.f / view_->GetDipScale(), env, motion_event.obj(), time_ms,
      android_action, pointer_count, history_size, action_index,
      0 /* action_button */, android_button_state, android_meta_state,
      raw_pos_x - pos_x_0, raw_pos_y - pos_y_0, &pointer0, &pointer1);
  return view_->OnTouchEvent(event, is_touch_handle_event);
}

void EventForwarder::OnMouseEvent(JNIEnv* env,
                                  const JavaParamRef<jobject>& obj,
                                  jlong time_ms,
                                  jint android_action,
                                  jfloat x,
                                  jfloat y,
                                  jint pointer_id,
                                  jfloat orientation,
                                  jfloat pressure,
                                  jfloat tilt,
                                  jint android_action_button,
                                  jint android_button_state,
                                  jint android_meta_state,
                                  jint android_tool_type) {
  // Construct a motion_event object minimally, only to convert the raw
  // parameters to ui::MotionEvent values. Since we used only the cached values
  // at index=0, it is okay to even pass a null event to the constructor.
  ui::MotionEventAndroid::Pointer pointer(
      pointer_id, x, y, 0.0f /* touch_major */, 0.0f /* touch_minor */,
      orientation, tilt, android_tool_type);
  ui::MotionEventAndroid event(
      1.f / view_->GetDipScale(), env, nullptr /* event */, time_ms,
      android_action, 1 /* pointer_count */, 0 /* history_size */,
      0 /* action_index */, android_action_button, android_button_state,
      android_meta_state, 0 /* raw_offset_x_pixels */,
      0 /* raw_offset_y_pixels */, &pointer, nullptr);
  view_->OnMouseEvent(event);
}

bool RegisterEventForwarder(JNIEnv* env) {
  return RegisterNativesImpl(env);
}

}  // namespace ui
