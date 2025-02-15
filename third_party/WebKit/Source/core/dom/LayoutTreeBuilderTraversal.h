/*
 * Copyright (C) 2012 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef LayoutTreeBuilderTraversal_h
#define LayoutTreeBuilderTraversal_h

#include "core/CoreExport.h"
#include "core/dom/Element.h"
#include "core/dom/shadow/InsertionPoint.h"
#include <cstdint>

namespace blink {

class LayoutObject;

class CORE_EXPORT LayoutTreeBuilderTraversal {
 public:
  static const int32_t kTraverseAllSiblings = -2;
  class ParentDetails {
    STACK_ALLOCATED();

   public:
    ParentDetails() : m_insertionPoint(nullptr) {}

    const InsertionPoint* insertionPoint() const { return m_insertionPoint; }

    void didTraverseInsertionPoint(const InsertionPoint*);

    bool operator==(const ParentDetails& other) {
      return m_insertionPoint == other.m_insertionPoint;
    }

   private:
    Member<const InsertionPoint> m_insertionPoint;
  };

  static ContainerNode* parent(const Node&, ParentDetails* = nullptr);
  static ContainerNode* layoutParent(const Node&, ParentDetails* = nullptr);
  static Node* firstChild(const Node&);
  static Node* nextSibling(const Node&);
  static Node* nextLayoutSibling(const Node& node) {
    int32_t limit = kTraverseAllSiblings;
    return nextLayoutSibling(node, limit);
  }
  static Node* previousLayoutSibling(const Node& node) {
    int32_t limit = kTraverseAllSiblings;
    return previousLayoutSibling(node, limit);
  }
  static Node* previousSibling(const Node&);
  static Node* previous(const Node&, const Node* stayWithin);
  static Node* next(const Node&, const Node* stayWithin);
  static Node* nextSkippingChildren(const Node&, const Node* stayWithin);
  static LayoutObject* parentLayoutObject(const Node&);
  static LayoutObject* nextSiblingLayoutObject(
      const Node&,
      int32_t limit = kTraverseAllSiblings);
  static LayoutObject* previousSiblingLayoutObject(
      const Node&,
      int32_t limit = kTraverseAllSiblings);
  static LayoutObject* nextInTopLayer(const Element&);

  static inline Element* parentElement(const Node& node) {
    ContainerNode* found = parent(node);
    return found && found->isElementNode() ? toElement(found) : 0;
  }

 private:
  static Node* nextLayoutSibling(const Node&, int32_t& limit);
  static Node* previousLayoutSibling(const Node&, int32_t& limit);
};

}  // namespace blink

#endif
