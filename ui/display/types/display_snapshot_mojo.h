// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_DISPLAY_DISPLAY_SNAPSHOT_MOJO_H_
#define UI_DISPLAY_DISPLAY_SNAPSHOT_MOJO_H_

#include <memory>

#include "base/macros.h"
#include "ui/display/types/display_snapshot.h"
#include "ui/display/types/display_types_export.h"

namespace display {

// DisplaySnapshot implementation that can be used with Mojo IPC.
class DISPLAY_TYPES_EXPORT DisplaySnapshotMojo : public DisplaySnapshot {
 public:
  DisplaySnapshotMojo(int64_t display_id,
                      const gfx::Point& origin,
                      const gfx::Size& physical_size,
                      DisplayConnectionType type,
                      bool is_aspect_preserving_scaling,
                      bool has_overscan,
                      bool has_color_correction_matrix,
                      std::string display_name,
                      const base::FilePath& sys_path,
                      int64_t product_id,
                      DisplayModeList modes,
                      const std::vector<uint8_t>& edid,
                      const DisplayMode* current_mode,
                      const DisplayMode* native_mode,
                      const gfx::Size& maximum_cursor_size);
  ~DisplaySnapshotMojo() override;
  std::unique_ptr<DisplaySnapshotMojo> Clone() const;

  // DisplaySnapshot:
  std::string ToString() const override;

 private:
  DISALLOW_COPY_AND_ASSIGN(DisplaySnapshotMojo);
};

}  // namespace display

#endif  // UI_DISPLAY_DISPLAY_SNAPSHOT_MOJO_H_
