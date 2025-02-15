// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview
 * 'settings-pointers' is the settings subpage with mouse and touchpad settings.
 */
Polymer({
  is: 'settings-pointers',

  properties: {
    prefs: {
      type: Object,
      notify: true,
    },

    hasMouse: Boolean,

    hasTouchpad: Boolean,

    /**
     * TODO(michaelpg): settings-slider should optionally take a min and max so
     * we don't have to generate a simple range of natural numbers ourselves.
     * @const {!Array<number>}
     * @private
     */
    sensitivityValues_: {
      type: Array,
      value: [1, 2, 3, 4, 5],
      readOnly: true,
    },
  },

  /**
   * Prevents the link from activating its parent paper-radio-button.
   * @param {!Event} e
   * @private
   */
  onLearnMoreLinkActivated_: function(e) {
    settings.DevicePageBrowserProxyImpl.getInstance().handleLinkEvent(e);
  },

  /**
   * Mouse and touchpad sections are only subsections if they are both present.
   * @param {boolean} hasMouse
   * @param {boolean} hasTouchpad
   * @return {string}
   */
  getSubsectionClass_: function(hasMouse, hasTouchpad) {
    return hasMouse && hasTouchpad ? 'subsection' : '';
  },
});
