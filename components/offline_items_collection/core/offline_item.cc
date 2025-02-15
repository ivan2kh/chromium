// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/offline_items_collection/core/offline_item.h"

namespace offline_items_collection {

ContentId::ContentId() = default;

ContentId::ContentId(const ContentId& other) = default;

ContentId::ContentId(const std::string& name_space, const std::string& id)
    : name_space(name_space), id(id) {}

ContentId::~ContentId() = default;

bool ContentId::operator==(const ContentId& content_id) const {
  return name_space == content_id.name_space && id == content_id.id;
}

bool ContentId::operator<(const ContentId& content_id) const {
  return std::tie(name_space, id) <
         std::tie(content_id.name_space, content_id.id);
}

OfflineItem::OfflineItem()
    : filter(OfflineItemFilter::FILTER_OTHER),
      total_size_bytes(0),
      externally_removed(false),
      is_off_the_record(false),
      state(OfflineItemState::COMPLETE),
      is_resumable(false),
      received_bytes(0),
      percent_completed(0),
      time_remaining_ms(0) {}

OfflineItem::OfflineItem(const OfflineItem& other) = default;

OfflineItem::OfflineItem(const ContentId& id) : OfflineItem() {
  this->id = id;
}

OfflineItem::~OfflineItem() = default;

bool OfflineItem::operator==(const OfflineItem& offline_item) const {
  return id == offline_item.id && title == offline_item.title &&
         description == offline_item.description &&
         filter == offline_item.filter &&
         total_size_bytes == offline_item.total_size_bytes &&
         externally_removed == offline_item.externally_removed &&
         creation_time == offline_item.creation_time &&
         last_accessed_time == offline_item.last_accessed_time &&
         page_url == offline_item.page_url &&
         original_url == offline_item.original_url &&
         is_off_the_record == offline_item.is_off_the_record &&
         state == offline_item.state &&
         is_resumable == offline_item.is_resumable &&
         received_bytes == offline_item.received_bytes &&
         percent_completed == offline_item.percent_completed &&
         time_remaining_ms == offline_item.time_remaining_ms;
}

}  // namespace offline_items_collection
