// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/layout/ng/ng_inline_node.h"

#include "core/layout/LayoutTestHelper.h"
#include "core/layout/ng/ng_constraint_space.h"
#include "core/layout/ng/ng_constraint_space_builder.h"
#include "core/layout/ng/ng_fragment_builder.h"
#include "core/layout/ng/ng_line_builder.h"
#include "core/layout/ng/ng_physical_box_fragment.h"
#include "core/layout/ng/ng_physical_text_fragment.h"
#include "core/layout/ng/ng_text_fragment.h"
#include "core/layout/ng/ng_text_layout_algorithm.h"
#include "core/style/ComputedStyle.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace blink {

class NGInlineNodeForTest : public NGInlineNode {
 public:
  using NGInlineNode::NGInlineNode;

  LayoutObject* GetLayoutObject() override { return nullptr; }

  String& Text() { return text_content_; }
  Vector<NGLayoutInlineItem>& Items() { return items_; }

  void Append(const String& text,
              const ComputedStyle* style = nullptr,
              LayoutObject* layout_object = nullptr) {
    unsigned start = text_content_.length();
    text_content_.append(text);
    items_.push_back(
        NGLayoutInlineItem(start, start + text.length(), style, layout_object));
  }

  void Append(UChar character) {
    text_content_.append(character);
    unsigned end = text_content_.length();
    items_.push_back(NGLayoutInlineItem(end - 1, end, nullptr));
    is_bidi_enabled_ = true;
  }

  void ClearText() {
    text_content_ = String();
    items_.clear();
  }

  void SegmentText() {
    is_bidi_enabled_ = true;
    NGInlineNode::SegmentText();
  }

  using NGInlineNode::ShapeText;
};

class NGInlineNodeTest : public RenderingTest {
 protected:
  void SetUp() override {
    RenderingTest::SetUp();
    style_ = ComputedStyle::create();
    style_->font().update(nullptr);
  }

  void SetupHtml(const char* id, const char* html) {
    setBodyInnerHTML(html);
    layout_block_flow_ = toLayoutBlockFlow(getLayoutObjectByElementId(id));
    layout_object_ = layout_block_flow_->firstChild();
    style_ = layout_object_->style();
  }

  void UseLayoutObjectAndAhem() {
    // Get Ahem from document. Loading "Ahem.woff" using |createTestFont| fails
    // on linux_chromium_asan_rel_ng.
    loadAhem();
    SetupHtml("t", "<div id=t style='font:10px Ahem'>test</div>");
  }

  NGInlineNodeForTest* CreateInlineNode() {
    if (!layout_block_flow_)
      SetupHtml("t", "<div id=t style='font:10px'>test</div>");
    return new NGInlineNodeForTest(layout_object_, layout_block_flow_);
  }

  void CreateLine(NGInlineNode* node,
                  Vector<RefPtr<const NGPhysicalTextFragment>>* fragments_out) {
    RefPtr<NGConstraintSpace> constraint_space =
        NGConstraintSpaceBuilder(kHorizontalTopBottom)
            .ToConstraintSpace(kHorizontalTopBottom);
    NGLineBuilder line_builder(node, constraint_space.get());

    NGTextLayoutAlgorithm algorithm(node);
    algorithm.LayoutInline(&line_builder);

    RefPtr<NGLayoutResult> result = line_builder.CreateFragments();
    for (const auto& child :
         toNGPhysicalBoxFragment(result->PhysicalFragment().get())
             ->Children()) {
      fragments_out->push_back(toNGPhysicalTextFragment(child.get()));
    }
  }

  RefPtr<const ComputedStyle> style_;
  LayoutBlockFlow* layout_block_flow_ = nullptr;
  LayoutObject* layout_object_ = nullptr;
  FontCachePurgePreventer purge_preventer_;
};

#define TEST_ITEM_OFFSET_DIR(item, start, end, direction) \
  EXPECT_EQ(start, item.StartOffset());                   \
  EXPECT_EQ(end, item.EndOffset());                       \
  EXPECT_EQ(direction, item.Direction())

TEST_F(NGInlineNodeTest, SegmentASCII) {
  NGInlineNodeForTest* node = CreateInlineNode();
  node->Append("Hello");
  node->SegmentText();
  Vector<NGLayoutInlineItem>& items = node->Items();
  ASSERT_EQ(1u, items.size());
  TEST_ITEM_OFFSET_DIR(items[0], 0u, 5u, TextDirection::kLtr);
}

TEST_F(NGInlineNodeTest, SegmentHebrew) {
  NGInlineNodeForTest* node = CreateInlineNode();
  node->Append(u"\u05E2\u05D1\u05E8\u05D9\u05EA");
  node->SegmentText();
  ASSERT_EQ(1u, node->Items().size());
  Vector<NGLayoutInlineItem>& items = node->Items();
  ASSERT_EQ(1u, items.size());
  TEST_ITEM_OFFSET_DIR(items[0], 0u, 5u, TextDirection::kRtl);
}

TEST_F(NGInlineNodeTest, SegmentSplit1To2) {
  NGInlineNodeForTest* node = CreateInlineNode();
  node->Append(u"Hello \u05E2\u05D1\u05E8\u05D9\u05EA");
  node->SegmentText();
  ASSERT_EQ(2u, node->Items().size());
  Vector<NGLayoutInlineItem>& items = node->Items();
  ASSERT_EQ(2u, items.size());
  TEST_ITEM_OFFSET_DIR(items[0], 0u, 6u, TextDirection::kLtr);
  TEST_ITEM_OFFSET_DIR(items[1], 6u, 11u, TextDirection::kRtl);
}

TEST_F(NGInlineNodeTest, SegmentSplit3To4) {
  NGInlineNodeForTest* node = CreateInlineNode();
  node->Append("Hel");
  node->Append(u"lo \u05E2");
  node->Append(u"\u05D1\u05E8\u05D9\u05EA");
  node->SegmentText();
  Vector<NGLayoutInlineItem>& items = node->Items();
  ASSERT_EQ(4u, items.size());
  TEST_ITEM_OFFSET_DIR(items[0], 0u, 3u, TextDirection::kLtr);
  TEST_ITEM_OFFSET_DIR(items[1], 3u, 6u, TextDirection::kLtr);
  TEST_ITEM_OFFSET_DIR(items[2], 6u, 7u, TextDirection::kRtl);
  TEST_ITEM_OFFSET_DIR(items[3], 7u, 11u, TextDirection::kRtl);
}

TEST_F(NGInlineNodeTest, SegmentBidiOverride) {
  NGInlineNodeForTest* node = CreateInlineNode();
  node->Append("Hello ");
  node->Append(rightToLeftOverrideCharacter);
  node->Append("ABC");
  node->Append(popDirectionalFormattingCharacter);
  node->SegmentText();
  Vector<NGLayoutInlineItem>& items = node->Items();
  ASSERT_EQ(4u, items.size());
  TEST_ITEM_OFFSET_DIR(items[0], 0u, 6u, TextDirection::kLtr);
  TEST_ITEM_OFFSET_DIR(items[1], 6u, 7u, TextDirection::kRtl);
  TEST_ITEM_OFFSET_DIR(items[2], 7u, 10u, TextDirection::kRtl);
  TEST_ITEM_OFFSET_DIR(items[3], 10u, 11u, TextDirection::kLtr);
}

static NGInlineNodeForTest* CreateBidiIsolateNode(NGInlineNodeForTest* node,
                                                  const ComputedStyle* style,
                                                  LayoutObject* layout_object) {
  node->Append("Hello ", style, layout_object);
  node->Append(rightToLeftIsolateCharacter);
  node->Append(u"\u05E2\u05D1\u05E8\u05D9\u05EA ", style, layout_object);
  node->Append(leftToRightIsolateCharacter);
  node->Append("A", style, layout_object);
  node->Append(popDirectionalIsolateCharacter);
  node->Append(u"\u05E2\u05D1\u05E8\u05D9\u05EA", style, layout_object);
  node->Append(popDirectionalIsolateCharacter);
  node->Append(" World", style, layout_object);
  node->SegmentText();
  return node;
}

TEST_F(NGInlineNodeTest, SegmentBidiIsolate) {
  NGInlineNodeForTest* node =
      CreateBidiIsolateNode(CreateInlineNode(), style_.get(), layout_object_);
  Vector<NGLayoutInlineItem>& items = node->Items();
  ASSERT_EQ(9u, items.size());
  TEST_ITEM_OFFSET_DIR(items[0], 0u, 6u, TextDirection::kLtr);
  TEST_ITEM_OFFSET_DIR(items[1], 6u, 7u, TextDirection::kLtr);
  TEST_ITEM_OFFSET_DIR(items[2], 7u, 13u, TextDirection::kRtl);
  TEST_ITEM_OFFSET_DIR(items[3], 13u, 14u, TextDirection::kRtl);
  TEST_ITEM_OFFSET_DIR(items[4], 14u, 15u, TextDirection::kLtr);
  TEST_ITEM_OFFSET_DIR(items[5], 15u, 16u, TextDirection::kRtl);
  TEST_ITEM_OFFSET_DIR(items[6], 16u, 21u, TextDirection::kRtl);
  TEST_ITEM_OFFSET_DIR(items[7], 21u, 22u, TextDirection::kLtr);
  TEST_ITEM_OFFSET_DIR(items[8], 22u, 28u, TextDirection::kLtr);
}

#define TEST_TEXT_FRAGMENT(fragment, node, index, start_offset, end_offset, \
                           dir)                                             \
  EXPECT_EQ(node, fragment->Node());                                        \
  EXPECT_EQ(index, fragment->ItemIndex());                                  \
  EXPECT_EQ(start_offset, fragment->StartOffset());                         \
  EXPECT_EQ(end_offset, fragment->EndOffset());                             \
  EXPECT_EQ(dir, node->Items()[fragment->ItemIndex()].Direction())

TEST_F(NGInlineNodeTest, CreateLineBidiIsolate) {
  UseLayoutObjectAndAhem();
  RefPtr<ComputedStyle> style = ComputedStyle::create();
  style->setLineHeight(Length(1, Fixed));
  style->font().update(nullptr);
  NGInlineNodeForTest* node =
      CreateBidiIsolateNode(CreateInlineNode(), style.get(), layout_object_);
  node->ShapeText();
  Vector<RefPtr<const NGPhysicalTextFragment>> fragments;
  CreateLine(node, &fragments);
  ASSERT_EQ(5u, fragments.size());
  TEST_TEXT_FRAGMENT(fragments[0], node, 0u, 0u, 6u, TextDirection::kLtr);
  TEST_TEXT_FRAGMENT(fragments[1], node, 6u, 16u, 21u, TextDirection::kRtl);
  TEST_TEXT_FRAGMENT(fragments[2], node, 4u, 14u, 15u, TextDirection::kLtr);
  TEST_TEXT_FRAGMENT(fragments[3], node, 2u, 7u, 13u, TextDirection::kRtl);
  TEST_TEXT_FRAGMENT(fragments[4], node, 8u, 22u, 28u, TextDirection::kLtr);
}

TEST_F(NGInlineNodeTest, MinMaxContentSize) {
  UseLayoutObjectAndAhem();
  NGInlineNodeForTest* node = CreateInlineNode();
  node->Append("AB CDE", style_.get(), layout_object_);
  node->ShapeText();
  MinMaxContentSize sizes = node->ComputeMinMaxContentSize();
  // TODO(kojii): min_content should be 20, but is 30 until NGLineBuilder
  // implements trailing spaces correctly.
  EXPECT_EQ(30, sizes.min_content);
  EXPECT_EQ(60, sizes.max_content);
}

TEST_F(NGInlineNodeTest, MinMaxContentSizeElementBoundary) {
  UseLayoutObjectAndAhem();
  NGInlineNodeForTest* node = CreateInlineNode();
  node->Append("A B", style_.get(), layout_object_);
  node->Append("C D", style_.get(), layout_object_);
  node->ShapeText();
  MinMaxContentSize sizes = node->ComputeMinMaxContentSize();
  // |min_content| should be the width of "BC" because there is an element
  // boundary between "B" and "C" but no break opportunities.
  // TODO(kojii): min_content should be 20, but is 30 until NGLineBuilder
  // implements trailing spaces correctly.
  EXPECT_EQ(30, sizes.min_content);
  EXPECT_EQ(60, sizes.max_content);
}

}  // namespace blink
