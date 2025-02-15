// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// For cs.chromium.org

let line = 0;

document.addEventListener('mousedown', (event) => {
  // right click
  if (event.button == 2) {
    let element = event.target;
    while (element != null && element.tagName == 'SPAN') {
      if (element.className == 'stx-line') {
        line = parseInt(element.id.split('_')[1]);
        break;
      } else {
        element = element.parentElement;
      }
    }
  }
}, true);

chrome.runtime.onMessage.addListener((request, sender, sendResponse) => {
  if (request == 'getLine') {
    sendResponse({line: line});
  }
});