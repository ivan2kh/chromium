/*
 * Copyright (C) 2013 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
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

#ifndef GridArea_h
#define GridArea_h

#include "core/style/GridPositionsResolver.h"
#include "wtf/Allocator.h"
#include "wtf/HashMap.h"
#include "wtf/MathExtras.h"
#include "wtf/text/WTFString.h"
#include <algorithm>

namespace blink {

// Recommended maximum size for both explicit and implicit grids. Note that this
// actually allows a [-9999,9999] range. The limit is low on purpouse because
// higher values easly trigger OOM situations. That will definitely improve once
// we switch from a vector of vectors based grid representation to a more
// efficient one memory-wise.
const int kGridMaxTracks = 1000;

// A span in a single direction (either rows or columns). Note that |startLine|
// and |endLine| are grid lines' indexes.
// Despite line numbers in the spec start in "1", the indexes here start in "0".
struct GridSpan {
  USING_FAST_MALLOC(GridSpan);

 public:
  static GridSpan untranslatedDefiniteGridSpan(int startLine, int endLine) {
    return GridSpan(startLine, endLine, UntranslatedDefinite);
  }

  static GridSpan translatedDefiniteGridSpan(size_t startLine, size_t endLine) {
    return GridSpan(startLine, endLine, TranslatedDefinite);
  }

  static GridSpan indefiniteGridSpan() { return GridSpan(0, 1, Indefinite); }

  bool operator==(const GridSpan& o) const {
    return m_type == o.m_type && m_startLine == o.m_startLine &&
           m_endLine == o.m_endLine;
  }

  size_t integerSpan() const {
    DCHECK(isTranslatedDefinite());
    DCHECK_GT(m_endLine, m_startLine);
    return m_endLine - m_startLine;
  }

  int untranslatedStartLine() const {
    DCHECK_EQ(m_type, UntranslatedDefinite);
    return m_startLine;
  }

  int untranslatedEndLine() const {
    DCHECK_EQ(m_type, UntranslatedDefinite);
    return m_endLine;
  }

  size_t startLine() const {
    DCHECK(isTranslatedDefinite());
    DCHECK_GE(m_startLine, 0);
    return m_startLine;
  }

  size_t endLine() const {
    DCHECK(isTranslatedDefinite());
    DCHECK_GT(m_endLine, 0);
    return m_endLine;
  }

  struct GridSpanIterator {
    GridSpanIterator(size_t v) : value(v) {}

    size_t operator*() const { return value; }
    size_t operator++() { return value++; }
    bool operator!=(GridSpanIterator other) const {
      return value != other.value;
    }

    size_t value;
  };

  GridSpanIterator begin() const {
    DCHECK(isTranslatedDefinite());
    return m_startLine;
  }

  GridSpanIterator end() const {
    DCHECK(isTranslatedDefinite());
    return m_endLine;
  }

  bool isTranslatedDefinite() const { return m_type == TranslatedDefinite; }

  bool isIndefinite() const { return m_type == Indefinite; }

  void translate(size_t offset) {
    DCHECK_EQ(m_type, UntranslatedDefinite);

    m_type = TranslatedDefinite;
    m_startLine += offset;
    m_endLine += offset;

    DCHECK_GE(m_startLine, 0);
    DCHECK_GT(m_endLine, 0);
  }

 private:
  enum GridSpanType { UntranslatedDefinite, TranslatedDefinite, Indefinite };

  GridSpan(int startLine, int endLine, GridSpanType type) : m_type(type) {
#if DCHECK_IS_ON()
    DCHECK_LT(startLine, endLine);
    if (type == TranslatedDefinite) {
      DCHECK_GE(startLine, 0);
      DCHECK_GT(endLine, 0);
    }
#endif

    m_startLine = clampTo<int>(startLine, -kGridMaxTracks, kGridMaxTracks - 1);
    m_endLine = clampTo<int>(endLine, -kGridMaxTracks + 1, kGridMaxTracks);
  }

  int m_startLine;
  int m_endLine;
  GridSpanType m_type;
};

// This represents a grid area that spans in both rows' and columns' direction.
struct GridArea {
  USING_FAST_MALLOC(GridArea);

 public:
  // HashMap requires a default constuctor.
  GridArea()
      : columns(GridSpan::indefiniteGridSpan()),
        rows(GridSpan::indefiniteGridSpan()) {}

  GridArea(const GridSpan& r, const GridSpan& c) : columns(c), rows(r) {}

  bool operator==(const GridArea& o) const {
    return columns == o.columns && rows == o.rows;
  }

  bool operator!=(const GridArea& o) const { return !(*this == o); }

  GridSpan columns;
  GridSpan rows;
};

typedef HashMap<String, GridArea> NamedGridAreaMap;

}  // namespace blink

#endif  // GridArea_h
