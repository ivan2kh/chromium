// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/payments/content/android/utility/fingerprint_parser.h"

#include "testing/gtest/include/gtest/gtest.h"

namespace payments {
namespace {

TEST(FingerprintParserTest, CheckInputSize) {
  // To short.
  EXPECT_TRUE(FingerprintStringToByteArray("00:01:02:03:04:05:06:07:08:09:"
                                           "A0:A1:A2:A3:A4:A5:A6:A7:A8:A9:"
                                           "B0:B1:B2:B3:B4:B5:B6:B7:B8:B9:"
                                           "C0:C")
                  .empty());
  EXPECT_TRUE(FingerprintStringToByteArray("00:01:02:03:04:05:06:07:08:09:"
                                           "A0:A1:A2:A3:A4:A5:A6:A7:A8:A9:"
                                           "B0:B1:B2:B3:B4:B5:B6:B7:B8:B9:"
                                           "C0:")
                  .empty());
  EXPECT_TRUE(FingerprintStringToByteArray("00:01:02:03:04:05:06:07:08:09:"
                                           "A0:A1:A2:A3:A4:A5:A6:A7:A8:A9:"
                                           "B0:B1:B2:B3:B4:B5:B6:B7:B8:B9:"
                                           "C0")
                  .empty());

  // To long.
  EXPECT_TRUE(FingerprintStringToByteArray("00:01:02:03:04:05:06:07:08:09:"
                                           "A0:A1:A2:A3:A4:A5:A6:A7:A8:A9:"
                                           "B0:B1:B2:B3:B4:B5:B6:B7:B8:B9:"
                                           "C0:C11")
                  .empty());
  EXPECT_TRUE(FingerprintStringToByteArray("00:01:02:03:04:05:06:07:08:09:"
                                           "A0:A1:A2:A3:A4:A5:A6:A7:A8:A9:"
                                           "B0:B1:B2:B3:B4:B5:B6:B7:B8:B9:"
                                           "C0:C1:")
                  .empty());
  EXPECT_TRUE(FingerprintStringToByteArray("00:01:02:03:04:05:06:07:08:09:"
                                           "A0:A1:A2:A3:A4:A5:A6:A7:A8:A9:"
                                           "B0:B1:B2:B3:B4:B5:B6:B7:B8:B9:"
                                           "C0:C1:C")
                  .empty());
  EXPECT_TRUE(FingerprintStringToByteArray("00:01:02:03:04:05:06:07:08:09:"
                                           "A0:A1:A2:A3:A4:A5:A6:A7:A8:A9:"
                                           "B0:B1:B2:B3:B4:B5:B6:B7:B8:B9:"
                                           "C0:C1:C2")
                  .empty());
}

TEST(FingerprintParserTest, CheckColonSeparator) {
  EXPECT_TRUE(FingerprintStringToByteArray("00-01-02-03-04-05-06-07-08-09-"
                                           "A0-A1-A2-A3-A4-A5-A6-A7-A8-A9-"
                                           "B0-B1-B2-B3-B4-B5-B6-B7-B8-B9-"
                                           "C0-C1")
                  .empty());
}

TEST(FingerprintParserTest, MustBeHex) {
  EXPECT_TRUE(FingerprintStringToByteArray("G0:G1:G2:G3:G4:G5:G6:G7:G8:G9:"
                                           "A0:A1:A2:A3:A4:A5:A6:A7:A8:A9:"
                                           "B0:B1:B2:B3:B4:B5:B6:B7:B8:B9:"
                                           "C0:C1")
                  .empty());
}

TEST(FingerprintParserTest, MustBeUpperCaseHex) {
  EXPECT_TRUE(FingerprintStringToByteArray("00:01:02:03:04:05:06:07:08:09:"
                                           "a0:a1:a2:a3:a4:a5:a6:a7:a8:a9:"
                                           "b0:b1:b2:b3:b4:b5:b6:b7:b8:b9:"
                                           "c0:c1")
                  .empty());
}

TEST(FingerprintParserTest, CorrectParsing) {
  std::vector<uint8_t> actual_output = FingerprintStringToByteArray(
      "00:01:02:03:04:05:06:07:08:09:A0:"
      "A1:A2:A3:A4:A5:A6:A7:A8:A9:B0:B1:"
      "B2:B3:B4:B5:B6:B7:B8:B9:FE:FF");
  std::vector<uint8_t> expect_output = {
      0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0xA0,
      0xA1, 0xA2, 0xA3, 0xA4, 0xA5, 0xA6, 0xA7, 0xA8, 0xA9, 0xB0, 0xB1,
      0xB2, 0xB3, 0xB4, 0xB5, 0xB6, 0xB7, 0xB8, 0xB9, 0xFE, 0xFF};
  EXPECT_EQ(expect_output, actual_output);
}

}  // namespace
}  // namespace payments
