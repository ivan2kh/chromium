// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.components.offline_items_collection;

import org.chromium.base.annotations.CalledByNative;
import org.chromium.base.annotations.JNINamespace;
import org.chromium.components.offline_items_collection.OfflineItemFilter.OfflineItemFilterEnum;
import org.chromium.components.offline_items_collection.OfflineItemState.OfflineItemStateEnum;

import java.util.ArrayList;

/**
 * The Java counterpart to the C++ class OfflineItemBridge
 * (components/offline_items_collection/core/android/offline_item_bridge.h).  This class has no
 * public members or methods and is meant as a private factory to build {@link OfflineItem}
 * instances.
 */
@JNINamespace("offline_items_collection::android")
public class OfflineItemBridge {
    private OfflineItemBridge() {}

    /**
     * This is a helper method to allow C++ to create an {@link ArrayList} to add
     * {@link OfflineItem}s to.
     * @return An {@link ArrayList} for {@link OfflineItem}s.
     */
    @CalledByNative
    private static ArrayList<OfflineItem> createArrayList() {
        return new ArrayList<OfflineItem>();
    }

    /**
     * Creates an {@link OfflineItem} from the passed in parameters.  See {@link OfflineItem} for a
     * list of the members that will be populated.  If {@code list} isn't {@code null}, the newly
     * created {@link OfflineItem} will be added to it.
     * @param list An {@link ArrayList} to optionally add the newly created {@link OfflineItem} to.
     * @return The newly created {@link OfflineItem} based on the passed in parameters.
     */
    @CalledByNative
    private static OfflineItem createOfflineItemAndMaybeAddToList(ArrayList<OfflineItem> list,
            String nameSpace, String id, String title, String description,
            @OfflineItemFilterEnum int filter, long totalSizeBytes, boolean externallyRemoved,
            long creationTimeMs, long lastAccessedTimeMs, String pageUrl, String originalUrl,
            boolean isOffTheRecord, @OfflineItemStateEnum int state, boolean isResumable,
            long receivedBytes, int percentCompleted, long timeRemainingMs) {
        OfflineItem item = new OfflineItem();
        item.id.namespace = nameSpace;
        item.id.id = id;
        item.title = title;
        item.description = description;
        item.filter = filter;
        item.totalSizeBytes = totalSizeBytes;
        item.externallyRemoved = externallyRemoved;
        item.creationTimeMs = creationTimeMs;
        item.lastAccessedTimeMs = lastAccessedTimeMs;
        item.pageUrl = pageUrl;
        item.originalUrl = originalUrl;
        item.isOffTheRecord = isOffTheRecord;
        item.state = state;
        item.isResumable = isResumable;
        item.receivedBytes = receivedBytes;
        item.percentCompleted = percentCompleted;
        item.timeRemainingMs = timeRemainingMs;

        if (list != null) list.add(item);
        return item;
    }
}