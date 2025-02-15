// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/doodle/doodle_types.h"

#include <memory>
#include <string>

#include "base/json/json_reader.h"
#include "base/optional.h"
#include "base/values.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

using testing::Eq;

namespace doodle {

namespace {

// Parses the given |json| string into a base::DictionaryValue and creates a
// DoodleImage out of that. |json| must be a valid json string.
base::Optional<DoodleImage> DoodleImageFromJson(
    const std::string& json,
    const base::Optional<GURL>& base_url) {
  std::unique_ptr<base::DictionaryValue> value =
      base::DictionaryValue::From(base::JSONReader::Read(json));
  DCHECK(value);
  return DoodleImage::FromDictionary(*value, base_url);
}

// Parses the given |json| string into a base::DictionaryValue and creates a
// DoodleConfig out of that. |json| must be a valid json string.
base::Optional<DoodleConfig> DoodleConfigFromJson(
    const std::string& json,
    const base::Optional<GURL>& base_url) {
  std::unique_ptr<base::DictionaryValue> value =
      base::DictionaryValue::From(base::JSONReader::Read(json));
  DCHECK(value);
  return DoodleConfig::FromDictionary(*value, base_url);
}

}  // namespace

TEST(DoodleImageTest, ParsesMinimalImage) {
  std::string json = R"json({
        "url":"https://www.doodle.com/doodle"
      })json";
  base::Optional<DoodleImage> image = DoodleImageFromJson(json, base::nullopt);
  ASSERT_TRUE(image.has_value());
  EXPECT_THAT(image->url, Eq(GURL("https://www.doodle.com/doodle")));
  EXPECT_THAT(image->height, Eq(0));
  EXPECT_THAT(image->width, Eq(0));
  EXPECT_THAT(image->is_animated_gif, Eq(false));
  EXPECT_THAT(image->is_cta, Eq(false));
}

TEST(DoodleImageTest, ParsesFullImage) {
  std::string json = R"json({
        "url":"https://www.doodle.com/doodle",
        "height":100,
        "width":50,
        "is_animated_gif":true,
        "is_cta":true
      })json";
  base::Optional<DoodleImage> image = DoodleImageFromJson(json, base::nullopt);
  ASSERT_TRUE(image.has_value());
  EXPECT_THAT(image->url, Eq(GURL("https://www.doodle.com/doodle")));
  EXPECT_THAT(image->height, Eq(100));
  EXPECT_THAT(image->width, Eq(50));
  EXPECT_THAT(image->is_animated_gif, Eq(true));
  EXPECT_THAT(image->is_cta, Eq(true));
}

TEST(DoodleImageTest, ResolvesRelativeUrl) {
  std::string json = R"json({
        "url":"/the_doodle_path"
      })json";
  base::Optional<DoodleImage> image =
      DoodleImageFromJson(json, GURL("https://doodle.dood/"));
  ASSERT_TRUE(image.has_value());
  EXPECT_THAT(image->url, Eq(GURL("https://doodle.dood/the_doodle_path")));
}

TEST(DoodleImageTest, DoesNotResolveAbsoluteUrl) {
  std::string json = R"json({
        "url":"https://www.doodle.com/the_doodle_path"
      })json";
  base::Optional<DoodleImage> image =
      DoodleImageFromJson(json, GURL("https://other.com/"));
  ASSERT_TRUE(image.has_value());
  EXPECT_THAT(image->url, Eq(GURL("https://www.doodle.com/the_doodle_path")));
}

TEST(DoodleImageTest, RequiresUrl) {
  std::string json = R"json({
        "height":100,
        "width":50,
        "is_animated_gif":true,
        "is_cta":true
      })json";
  base::Optional<DoodleImage> image = DoodleImageFromJson(json, base::nullopt);
  EXPECT_FALSE(image.has_value());
}

TEST(DoodleImageTest, HandlesInvalidUrl) {
  std::string json = R"json({
        "url":"not_a_url"
      })json";
  base::Optional<DoodleImage> image = DoodleImageFromJson(json, base::nullopt);
  // The URL is required, and invalid should be treated like missing.
  EXPECT_FALSE(image.has_value());
}

TEST(DoodleImageTest, PreservesFieldsOverRoundtrip) {
  // Set all fields to non-default values.
  DoodleImage image(GURL("https://www.doodle.com/doodle"));
  image.height = 100;
  image.width = 50;
  image.is_animated_gif = true;
  image.is_cta = true;

  // Convert to a dictionary and back.
  base::Optional<DoodleImage> after_roundtrip =
      DoodleImage::FromDictionary(*image.ToDictionary(), base::nullopt);

  // Make sure everything survived.
  ASSERT_TRUE(after_roundtrip.has_value());
  EXPECT_THAT(*after_roundtrip, Eq(image));
}

TEST(DoodleConfigTest, ParsesMinimalConfig) {
  std::string json = R"json({
        "large_image":{"url":"https://doodle.com/img.jpg"}
      })json";
  base::Optional<DoodleConfig> config =
      DoodleConfigFromJson(json, base::nullopt);
  ASSERT_TRUE(config.has_value());
  EXPECT_THAT(config->doodle_type, Eq(DoodleType::UNKNOWN));
  EXPECT_THAT(config->alt_text, Eq(std::string()));
  EXPECT_THAT(config->interactive_html, Eq(std::string()));
  EXPECT_THAT(config->search_url, Eq(GURL()));
  EXPECT_THAT(config->target_url, Eq(GURL()));
  EXPECT_THAT(config->fullpage_interactive_url, Eq(GURL()));
  EXPECT_FALSE(config->large_cta_image.has_value());
  EXPECT_FALSE(config->transparent_large_image.has_value());
}

TEST(DoodleConfigTest, ParsesFullConfig) {
  std::string json = R"json({
        "doodle_type":"SLIDESHOW",
        "alt_text":"some text",
        "interactive_html":"<div id='dood'></div>",
        "search_url":"https://doodle.com/search",
        "target_url":"https://doodle.com/target",
        "fullpage_interactive_url":"https://doodle.com/interactive",
        "large_image":{"url":"https://doodle.com/img.jpg"},
        "large_cta_image":{"url":"https://doodle.com/cta.jpg"},
        "transparent_large_image":{"url":"https://doodle.com/transparent.jpg"}
      })json";
  base::Optional<DoodleConfig> config =
      DoodleConfigFromJson(json, base::nullopt);
  ASSERT_TRUE(config.has_value());
  EXPECT_THAT(config->doodle_type, Eq(DoodleType::SLIDESHOW));
  EXPECT_THAT(config->alt_text, Eq("some text"));
  EXPECT_THAT(config->interactive_html, Eq("<div id='dood'></div>"));
  EXPECT_THAT(config->search_url, Eq(GURL("https://doodle.com/search")));
  EXPECT_THAT(config->target_url, Eq(GURL("https://doodle.com/target")));
  EXPECT_THAT(config->fullpage_interactive_url,
              Eq(GURL("https://doodle.com/interactive")));
  EXPECT_THAT(config->large_image.url, Eq(GURL("https://doodle.com/img.jpg")));
  ASSERT_TRUE(config->large_cta_image.has_value());
  EXPECT_THAT(config->large_cta_image->url,
              Eq(GURL("https://doodle.com/cta.jpg")));
  ASSERT_TRUE(config->transparent_large_image.has_value());
  EXPECT_THAT(config->transparent_large_image->url,
              Eq(GURL("https://doodle.com/transparent.jpg")));
}

TEST(DoodleConfigTest, RequiresLargeImage) {
  std::string json = R"json({
        "doodle_type":"SLIDESHOW",
        "alt_text":"some text",
        "interactive_html":"<div id='dood'></div>",
        "search_url":"https://doodle.com/search",
        "target_url":"https://doodle.com/target",
        "fullpage_interactive_url":"https://doodle.com/interactive",
        "large_cta_image":{"url":"https://doodle.com/cta.jpg"},
        "transparent_large_image":{"url":"https://doodle.com/transparent.jpg"}
      })json";
  base::Optional<DoodleConfig> config =
      DoodleConfigFromJson(json, base::nullopt);
  EXPECT_FALSE(config.has_value());
}

TEST(DoodleConfigTest, RequiresValidLargeImage) {
  std::string json = R"json({
        "doodle_type":"SLIDESHOW",
        "alt_text":"some text",
        "interactive_html":"<div id='dood'></div>",
        "search_url":"https://doodle.com/search",
        "target_url":"https://doodle.com/target",
        "fullpage_interactive_url":"https://doodle.com/interactive",
        "large_image":{"no_url":"asdf"},
        "large_cta_image":{"url":"https://doodle.com/cta.jpg"},
        "transparent_large_image":{"url":"https://doodle.com/transparent.jpg"}
      })json";
  base::Optional<DoodleConfig> config =
      DoodleConfigFromJson(json, base::nullopt);
  EXPECT_FALSE(config.has_value());
}

TEST(DoodleConfigTest, ResolvesRelativeUrls) {
  std::string json = R"json({
        "search_url":"/search",
        "target_url":"/target",
        "fullpage_interactive_url":"/interactive",
        "large_image":{"url":"/large.jpg"},
        "large_cta_image":{"url":"/cta.jpg"},
        "transparent_large_image":{"url":"/transparent.jpg"}
      })json";
  base::Optional<DoodleConfig> config =
      DoodleConfigFromJson(json, GURL("https://doodle.com/"));
  ASSERT_TRUE(config.has_value());
  EXPECT_THAT(config->search_url, Eq(GURL("https://doodle.com/search")));
  EXPECT_THAT(config->target_url, Eq(GURL("https://doodle.com/target")));
  EXPECT_THAT(config->fullpage_interactive_url,
              Eq(GURL("https://doodle.com/interactive")));
  EXPECT_THAT(config->large_image.url,
              Eq(GURL("https://doodle.com/large.jpg")));
  ASSERT_TRUE(config->large_cta_image.has_value());
  EXPECT_THAT(config->large_cta_image->url,
              Eq(GURL("https://doodle.com/cta.jpg")));
  ASSERT_TRUE(config->transparent_large_image.has_value());
  EXPECT_THAT(config->transparent_large_image->url,
              Eq(GURL("https://doodle.com/transparent.jpg")));
}

TEST(DoodleConfigTest, HandlesInvalidUrls) {
  std::string json = R"json({
        "search_url":"not_a_url",
        "target_url":"not_a_url",
        "fullpage_interactive_url":"not_a_url",
        "large_image":{"url":"https://doodle.com/img.jpg"}
      })json";
  base::Optional<DoodleConfig> config =
      DoodleConfigFromJson(json, base::nullopt);
  // All the URLs are optional, so invalid ones shouldn't matter.
  ASSERT_TRUE(config.has_value());
  EXPECT_TRUE(config->search_url.is_empty());
  EXPECT_TRUE(config->target_url.is_empty());
  EXPECT_TRUE(config->fullpage_interactive_url.is_empty());
}

TEST(DoodleConfigTest, PreservesFieldsOverRoundtrip) {
  // Set all fields to non-default values.
  DoodleConfig config(DoodleType::SLIDESHOW,
                      DoodleImage(GURL("https://www.doodle.com/img.jpg")));
  config.alt_text = "some text";
  config.interactive_html = "<div id='dood'></div>";
  config.search_url = GURL("https://doodle.com/search");
  config.target_url = GURL("https://doodle.com/target");
  config.fullpage_interactive_url = GURL("https://doodle.com/interactive");
  config.large_cta_image = DoodleImage(GURL("https://www.doodle.com/cta.jpg"));
  config.transparent_large_image =
      DoodleImage(GURL("https://www.doodle.com/transparent.jpg"));

  // Convert to a dictionary and back.
  // Note: The different |base_url| should make no difference, since all
  // persisted URLs are absolute already.
  base::Optional<DoodleConfig> after_roundtrip = DoodleConfig::FromDictionary(
      *config.ToDictionary(), GURL("https://other.com"));

  // Make sure everything survived.
  ASSERT_TRUE(after_roundtrip.has_value());
  EXPECT_THAT(*after_roundtrip, Eq(config));
}

}  // namespace doodle
