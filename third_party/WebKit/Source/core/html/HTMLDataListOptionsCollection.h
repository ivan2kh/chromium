// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef HTMLDataListOptionsCollection_h
#define HTMLDataListOptionsCollection_h

#include "core/html/HTMLCollection.h"
#include "core/html/HTMLOptionElement.h"

namespace blink {

class HTMLDataListOptionsCollection : public HTMLCollection {
 public:
  static HTMLDataListOptionsCollection* create(ContainerNode& ownerNode,
                                               CollectionType type) {
    DCHECK_EQ(type, DataListOptions);
    return new HTMLDataListOptionsCollection(ownerNode);
  }

  HTMLOptionElement* item(unsigned offset) const {
    return toHTMLOptionElement(HTMLCollection::item(offset));
  }

  bool elementMatches(const HTMLElement&) const;

 private:
  explicit HTMLDataListOptionsCollection(ContainerNode& ownerNode)
      : HTMLCollection(ownerNode, DataListOptions, DoesNotOverrideItemAfter) {}
};

DEFINE_TYPE_CASTS(HTMLDataListOptionsCollection,
                  LiveNodeListBase,
                  collection,
                  collection->type() == DataListOptions,
                  collection.type() == DataListOptions);

inline bool HTMLDataListOptionsCollection::elementMatches(
    const HTMLElement& element) const {
  return isHTMLOptionElement(element);
}

}  // namespace blink

#endif  // HTMLDataListOptionsCollection_h
