// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_RENDERER_MEDIA_MEDIA_STREAM_CONSTRAINTS_UTIL_VIDEO_DEVICE_H_
#define CONTENT_RENDERER_MEDIA_MEDIA_STREAM_CONSTRAINTS_UTIL_VIDEO_DEVICE_H_

#include <string>
#include <vector>

#include "base/logging.h"
#include "content/common/content_export.h"
#include "content/common/media/media_devices.mojom.h"
#include "media/capture/video_capture_types.h"
#include "third_party/webrtc/base/optional.h"

namespace blink {
class WebString;
class WebMediaConstraints;
}

namespace content {

// Calculates and returns videoKind value for |format|.
// See https://w3c.github.io/mediacapture-depth.
blink::WebString CONTENT_EXPORT
GetVideoKindForFormat(const media::VideoCaptureFormat& format);

struct CONTENT_EXPORT VideoDeviceCaptureCapabilities {
  VideoDeviceCaptureCapabilities();
  VideoDeviceCaptureCapabilities(VideoDeviceCaptureCapabilities&& other);
  ~VideoDeviceCaptureCapabilities();
  VideoDeviceCaptureCapabilities& operator=(
      VideoDeviceCaptureCapabilities&& other);

  // Each field is independent of each other.
  std::vector<::mojom::VideoInputDeviceCapabilitiesPtr> device_capabilities;
  std::vector<media::PowerLineFrequency> power_line_capabilities;
  std::vector<rtc::Optional<bool>> noise_reduction_capabilities;
};

class CONTENT_EXPORT VideoDeviceCaptureSourceSelectionResult {
 public:
  // Creates a result without value and with an empty failed constraint name.
  VideoDeviceCaptureSourceSelectionResult();

  // Creates a result without value and with the given |failed_constraint_name|.
  // Does not take ownership of |failed_constraint_name|, so it must be null or
  // point to a string that remains accessible.
  explicit VideoDeviceCaptureSourceSelectionResult(
      const char* failed_constraint_name);

  // Creates a result with the given values.
  VideoDeviceCaptureSourceSelectionResult(
      const std::string& device_id,
      ::mojom::FacingMode facing_mode_,
      media::VideoCaptureParams capture_params_,
      rtc::Optional<bool> noise_reduction_);

  VideoDeviceCaptureSourceSelectionResult(
      const VideoDeviceCaptureSourceSelectionResult& other);
  VideoDeviceCaptureSourceSelectionResult& operator=(
      const VideoDeviceCaptureSourceSelectionResult& other);
  VideoDeviceCaptureSourceSelectionResult(
      VideoDeviceCaptureSourceSelectionResult&& other);
  VideoDeviceCaptureSourceSelectionResult& operator=(
      VideoDeviceCaptureSourceSelectionResult&& other);
  ~VideoDeviceCaptureSourceSelectionResult();

  bool HasValue() const { return failed_constraint_name_ == nullptr; }

  // Convenience accessors for fields embedded in |capture_params|.
  const media::VideoCaptureFormat& Format() const {
    return capture_params_.requested_format;
  }
  int Width() const {
    DCHECK(HasValue());
    return capture_params_.requested_format.frame_size.width();
  }
  int Height() const {
    DCHECK(HasValue());
    return capture_params_.requested_format.frame_size.height();
  }
  float FrameRate() const {
    DCHECK(HasValue());
    return capture_params_.requested_format.frame_rate;
  }
  media::PowerLineFrequency PowerLineFrequency() const {
    DCHECK(HasValue());
    return capture_params_.power_line_frequency;
  }

  // Other accessors.
  const char* failed_constraint_name() const { return failed_constraint_name_; }

  const std::string& device_id() const {
    DCHECK(HasValue());
    return device_id_;
  }

  ::mojom::FacingMode facing_mode() const {
    DCHECK(HasValue());
    return facing_mode_;
  }

  const media::VideoCaptureParams& capture_params() const {
    DCHECK(HasValue());
    return capture_params_;
  }

  const rtc::Optional<bool>& noise_reduction() const {
    DCHECK(HasValue());
    return noise_reduction_;
  }

 private:
  const char* failed_constraint_name_;
  std::string device_id_;
  ::mojom::FacingMode facing_mode_;
  media::VideoCaptureParams capture_params_;
  rtc::Optional<bool> noise_reduction_;
};

// This function performs source and source-settings selection based on
// the given |capabilities| and |constraints|.
// Chromium performs constraint resolution in two steps. First, a source and its
// settings are selected; then a track is created, connected to the source, and
// finally the track settings are selected. This function implements an
// algorithm for the first step. Sources are not a user-visible concept, so the
// spec only specifies an algorithm for track settings.
// This algorithm for sources is compatible with the spec algorithm for tracks,
// as defined in https://w3c.github.io/mediacapture-main/#dfn-selectsettings,
// but it is customized to account for differences between sources and tracks,
// and to break ties when multiple source settings are equally good according to
// the spec algorithm.
// The main difference between a source and a track with regards to the spec
// algorithm is that a candidate  source can support a range of values for some
// constraints while a candidate track supports a single value. For example,
// cropping allows a source with native resolution AxB to support the range of
// resolutions from 1x1 to AxB.
// Only candidates that satisfy the basic constraint set are valid. If no
// candidate can satisfy the basic constraint set, this function returns
// a result without a valid |settings| field and with the name of a failed
// constraint in field |failed_constraint_name|. If at least one candidate that
// satisfies the basic constraint set can be found, this function returns a
// result with a valid |settings| field and a null |failed_constraint_name|.
// If there are no candidates at all, this function returns a result with an
// empty string in |failed_constraint_name| and without a valid |settings|
// field.
// The criteria to decide if a valid candidate is better than another one are as
// follows:
// 1. Given advanced constraint sets A[0],A[1]...,A[n], candidate C1 is better
//    than candidate C2 if C1 supports the first advanced set for which C1's
//    support is different than C2's support.
//    Examples:
//    * One advanced set, C1 supports it, and C2 does not. C1 is better.
//    * Two sets, C1 supports both, C2 supports only the first. C1 is better.
//    * Three sets, C1 supports the first and second set, C2 supports the first
//      and third set. C1 is better.
// 2. C1 is better than C2 if C1 has a smaller fitness distance than C2. The
//    fitness distance depends on the ability of the candidate to support ideal
//    values in the basic constraint set. This is the final criterion defined in
//    the spec.
// 3. C1 is better than C2 if C1 has a lower Chromium-specific custom distance
//    from the basic constraint set. This custom distance is the sum of various
//    constraint-specific custom distances.
//    For example, if the constraint set specifies a resolution of exactly
//    1000x1000 for a track, then a candidate with a resolution of 1200x1200
//    is better than a candidate with a resolution of 2000x2000. Both settings
//    satisfy the constraint set because cropping can be used to produce the
//    track setting of 1000x1000, but 1200x1200 is considered better because it
//    has lower resource usage. The same criteria applies for each advanced
//    constraint set.
// 4. C1 is better than C2 if its native settings have a smaller fitness
//    distance. For example, if the ideal resolution is 1000x1000 and C1 has a
//    native resolution of 1200x1200, while C2 has a native resolution of
//    2000x2000, then C1 is better because it can support the ideal value with
//    lower resource usage. Both C1 and C2 are better than a candidate C3 with
//    a native resolution of 999x999, since C3 has a nonzero distance to the
//    ideal value and thus has worse fitness according to step 2, even if C3's
//    native fitness is better than C1's and C2's.
// 5. C1 is better than C2 if its settings are closer to certain default
//    settings that include the device ID, power-line frequency, noise
//    reduction, resolution, and frame rate, in that order. Note that there is
//    no default facing mode or aspect ratio.
VideoDeviceCaptureSourceSelectionResult CONTENT_EXPORT
SelectVideoDeviceCaptureSourceSettings(
    const VideoDeviceCaptureCapabilities& capabilities,
    const blink::WebMediaConstraints& constraints);

}  // namespace content

#endif  // CONTENT_RENDERER_MEDIA_MEDIA_STREAM_CONSTRAINTS_UTIL_VIDEO_DEVICE_H_
