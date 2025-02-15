/*
 * This file is part of the DOM implementation for WebCore.
 *
 * Copyright (C) 2006 Apple Computer, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 */

#ifndef DocumentMarker_h
#define DocumentMarker_h

#include "core/CoreExport.h"
#include "platform/graphics/Color.h"
#include "platform/heap/Handle.h"
#include "wtf/VectorTraits.h"
#include "wtf/text/WTFString.h"

namespace blink {

class DocumentMarkerDetails;

// A range of a node within a document that is "marked", such as the range of a
// misspelled word. It optionally includes a description that could be displayed
// in the user interface. It also optionally includes a flag specifying whether
// the match is active, which is ignored for all types other than type
// TextMatch.
class CORE_EXPORT DocumentMarker : public GarbageCollected<DocumentMarker> {
 public:
  enum MarkerTypeIndex {
    SpellingMarkerIndex = 0,
    GrammarMarkerIndex,
    TextMatchMarkerIndex,
    CompositionMarkerIndex,
    MarkerTypeIndexesCount
  };

  enum MarkerType {
    Spelling = 1 << SpellingMarkerIndex,
    Grammar = 1 << GrammarMarkerIndex,
    TextMatch = 1 << TextMatchMarkerIndex,
    Composition = 1 << CompositionMarkerIndex,
  };

  class MarkerTypes {
   public:
    // The constructor is intentionally implicit to allow conversion from the
    // bit-wise sum of above types
    MarkerTypes(unsigned mask) : m_mask(mask) {}

    bool contains(MarkerType type) const { return m_mask & type; }
    bool intersects(const MarkerTypes& types) const {
      return (m_mask & types.m_mask);
    }
    bool operator==(const MarkerTypes& other) const {
      return m_mask == other.m_mask;
    }

    void add(const MarkerTypes& types) { m_mask |= types.m_mask; }
    void remove(const MarkerTypes& types) { m_mask &= ~types.m_mask; }

   private:
    unsigned m_mask;
  };

  class AllMarkers : public MarkerTypes {
   public:
    AllMarkers() : MarkerTypes(Spelling | Grammar | TextMatch | Composition) {}
  };

  class MisspellingMarkers : public MarkerTypes {
   public:
    MisspellingMarkers() : MarkerTypes(Spelling | Grammar) {}
  };

  class SpellCheckClientMarkers : public MarkerTypes {
   public:
    SpellCheckClientMarkers() : MarkerTypes(Spelling | Grammar) {}
  };

  DocumentMarker(MarkerType,
                 unsigned startOffset,
                 unsigned endOffset,
                 const String& description);
  DocumentMarker(unsigned startOffset, unsigned endOffset, bool activeMatch);
  DocumentMarker(unsigned startOffset,
                 unsigned endOffset,
                 Color underlineColor,
                 bool thick,
                 Color backgroundColor);

  DocumentMarker(const DocumentMarker&);

  MarkerType type() const { return m_type; }
  unsigned startOffset() const { return m_startOffset; }
  unsigned endOffset() const { return m_endOffset; }

  const String& description() const;
  bool activeMatch() const;
  Color underlineColor() const;
  bool thick() const;
  Color backgroundColor() const;
  DocumentMarkerDetails* details() const;

  void setActiveMatch(bool);
  void clearDetails() { m_details.clear(); }

  // Offset modifications are done by DocumentMarkerController.
  // Other classes should not call following setters.
  void setStartOffset(unsigned offset) { m_startOffset = offset; }
  void setEndOffset(unsigned offset) { m_endOffset = offset; }
  void shiftOffsets(int delta);

  bool operator==(const DocumentMarker& o) const {
    return type() == o.type() && startOffset() == o.startOffset() &&
           endOffset() == o.endOffset();
  }

  bool operator!=(const DocumentMarker& o) const { return !(*this == o); }

  DECLARE_TRACE();

 private:
  MarkerType m_type;
  unsigned m_startOffset;
  unsigned m_endOffset;
  Member<DocumentMarkerDetails> m_details;
};

using DocumentMarkerVector = HeapVector<Member<DocumentMarker>>;

inline DocumentMarkerDetails* DocumentMarker::details() const {
  return m_details.get();
}

class DocumentMarkerDetails
    : public GarbageCollectedFinalized<DocumentMarkerDetails> {
 public:
  DocumentMarkerDetails() {}
  virtual ~DocumentMarkerDetails();
  virtual bool isDescription() const { return false; }
  virtual bool isTextMatch() const { return false; }
  virtual bool isComposition() const { return false; }

  DEFINE_INLINE_VIRTUAL_TRACE() {}
};

}  // namespace blink

#endif  // DocumentMarker_h
