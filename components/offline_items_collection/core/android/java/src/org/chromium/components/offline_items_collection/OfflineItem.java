// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.components.offline_items_collection;

import org.chromium.base.annotations.SuppressFBWarnings;
import org.chromium.components.offline_items_collection.OfflineItemFilter.OfflineItemFilterEnum;
import org.chromium.components.offline_items_collection.OfflineItemState.OfflineItemStateEnum;

/**
 * This class is the Java counterpart to the C++ OfflineItem
 * (components/offline_items_collection/core/offline_item.h) class.
 *
 * For all member variable descriptions see the C++ class.
 * TODO(dtrainor): Investigate making all class members for this and the C++ counterpart const.
 */
@SuppressFBWarnings("URF_UNREAD_PUBLIC_OR_PROTECTED_FIELD")
public class OfflineItem {
    public ContentId id;

    // Display metadata.
    public String title;
    public String description;
    @OfflineItemFilterEnum
    public int filter;

    // Content Metadata.
    public long totalSizeBytes;
    public boolean externallyRemoved;
    public long creationTimeMs;
    public long lastAccessedTimeMs;

    // Request Metadata.
    public String pageUrl;
    public String originalUrl;
    public boolean isOffTheRecord;

    // In Progress Metadata.
    @OfflineItemStateEnum
    public int state;
    public boolean isResumable;
    public long receivedBytes;
    public int percentCompleted;
    public long timeRemainingMs;

    OfflineItem() {
        id = new ContentId();
        filter = OfflineItemFilter.FILTER_OTHER;
        state = OfflineItemState.COMPLETE;
    }
}