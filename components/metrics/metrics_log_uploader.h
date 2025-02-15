// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_METRICS_METRICS_LOG_UPLOADER_H_
#define COMPONENTS_METRICS_METRICS_LOG_UPLOADER_H_

#include <string>

#include "base/callback.h"
#include "base/macros.h"
#include "base/strings/string_piece.h"

namespace metrics {

// MetricsLogUploader is an abstract base class for uploading UMA logs on behalf
// of MetricsService.
class MetricsLogUploader {
 public:
  // Possible service types. This should correspond to a type from
  // DataUseUserData.
  enum MetricServiceType {
    UMA,
    UKM,
  };

  virtual ~MetricsLogUploader() {}

  // Uploads a log with the specified |compressed_log_data| and |log_hash|.
  // |log_hash| is expected to be the hex-encoded SHA1 hash of the log data
  // before compression.
  virtual void UploadLog(const std::string& compressed_log_data,
                         const std::string& log_hash) = 0;
};

}  // namespace metrics

#endif  // COMPONENTS_METRICS_METRICS_LOG_UPLOADER_H_
