// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/android/resources/nine_patch_resource.h"

#include "base/memory/ptr_util.h"

namespace ui {

// static
NinePatchResource* NinePatchResource::From(Resource* resource) {
  DCHECK_EQ(Type::NINE_PATCH_BITMAP, resource->type());
  return static_cast<NinePatchResource*>(resource);
}

NinePatchResource::NinePatchResource(gfx::Rect padding, gfx::Rect aperture)
    : Resource(Type::NINE_PATCH_BITMAP),
      padding_(padding),
      aperture_(aperture) {}

NinePatchResource::~NinePatchResource() = default;

gfx::Rect NinePatchResource::Border(const gfx::Size& bounds) const {
  return Border(bounds, gfx::InsetsF(1.f, 1.f, 1.f, 1.f));
}

gfx::Rect NinePatchResource::Border(const gfx::Size& bounds,
                                    const gfx::InsetsF& scale) const {
  // Calculate whether or not we need to scale down the border if the bounds of
  // the layer are going to be smaller than the aperture padding.
  float x_scale = std::min((float)bounds.width() / size().width(), 1.f);
  float y_scale = std::min((float)bounds.height() / size().height(), 1.f);

  float left_scale = std::min(x_scale * scale.left(), 1.f);
  float right_scale = std::min(x_scale * scale.right(), 1.f);
  float top_scale = std::min(y_scale * scale.top(), 1.f);
  float bottom_scale = std::min(y_scale * scale.bottom(), 1.f);

  return gfx::Rect(aperture_.x() * left_scale, aperture_.y() * top_scale,
                   (size().width() - aperture_.width()) * right_scale,
                   (size().height() - aperture_.height()) * bottom_scale);
}

std::unique_ptr<Resource> NinePatchResource::CreateForCopy() {
  return base::MakeUnique<NinePatchResource>(padding_, aperture_);
}

}  // namespace ui
