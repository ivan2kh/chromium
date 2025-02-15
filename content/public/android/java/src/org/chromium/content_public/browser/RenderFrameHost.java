// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.content_public.browser;

/**
 * The RenderFrameHost Java wrapper to allow communicating with the native RenderFrameHost object.
 *
 */
public interface RenderFrameHost {
    /**
     * Get the last committed URL of the frame.
     *
     * @return The last committed URL of the frame.
     */
    String getLastCommittedURL();
}
