// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/media_recorder/video_track_recorder.h"

#include <stddef.h>

#include <memory>

#include "base/bind.h"
#include "base/location.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/run_loop.h"
#include "base/strings/utf_string_conversions.h"
#include "content/child/child_process.h"
#include "content/renderer/media/media_stream_video_track.h"
#include "content/renderer/media/mock_media_stream_video_source.h"
#include "media/base/video_frame.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/WebKit/public/platform/WebString.h"
#include "third_party/WebKit/public/web/WebHeap.h"

using media::VideoFrame;
using video_track_recorder::kVEAEncoderMinResolutionWidth;
using video_track_recorder::kVEAEncoderMinResolutionHeight;

using ::testing::_;
using ::testing::DoAll;
using ::testing::InSequence;
using ::testing::Mock;
using ::testing::Return;
using ::testing::SaveArg;
using ::testing::TestWithParam;
using ::testing::ValuesIn;

namespace content {

ACTION_P(RunClosure, closure) {
  closure.Run();
}

const VideoTrackRecorder::CodecId kTrackRecorderTestCodec[] = {
    VideoTrackRecorder::CodecId::VP8,
    VideoTrackRecorder::CodecId::VP9
#if BUILDFLAG(RTC_USE_H264)
    , VideoTrackRecorder::CodecId::H264
#endif
};
const gfx::Size kTrackRecorderTestSize[] = {
    gfx::Size(kVEAEncoderMinResolutionWidth / 2,
              kVEAEncoderMinResolutionHeight / 2),
    gfx::Size(kVEAEncoderMinResolutionWidth, kVEAEncoderMinResolutionHeight)};
static const int kTrackRecorderTestSizeDiff = 20;

class VideoTrackRecorderTest
    : public TestWithParam<
          testing::tuple<VideoTrackRecorder::CodecId, gfx::Size, bool>> {
 public:
  VideoTrackRecorderTest()
      : mock_source_(new MockMediaStreamVideoSource(false)) {
    const blink::WebString webkit_track_id(
        blink::WebString::fromASCII("dummy"));
    blink_source_.initialize(webkit_track_id,
                             blink::WebMediaStreamSource::TypeVideo,
                             webkit_track_id);
    blink_source_.setExtraData(mock_source_);
    blink_track_.initialize(blink_source_);

    blink::WebMediaConstraints constraints;
    constraints.initialize();
    track_ = new MediaStreamVideoTrack(mock_source_, constraints,
                                       MediaStreamSource::ConstraintsCallback(),
                                       true /* enabled */);
    blink_track_.setTrackData(track_);

    // Paranoia checks.
    EXPECT_EQ(blink_track_.source().getExtraData(),
              blink_source_.getExtraData());
    EXPECT_TRUE(message_loop_.IsCurrent());
  }

  ~VideoTrackRecorderTest() {
    blink_track_.reset();
    blink_source_.reset();
    video_track_recorder_.reset();
    blink::WebHeap::collectAllGarbageForTesting();
  }

  void InitializeRecorder(VideoTrackRecorder::CodecId codec) {
    video_track_recorder_.reset(new VideoTrackRecorder(
        codec, blink_track_,
        base::Bind(&VideoTrackRecorderTest::OnEncodedVideo,
                   base::Unretained(this)),
        0 /* bits_per_second */));
  }

  MOCK_METHOD5(DoOnEncodedVideo,
               void(const media::WebmMuxer::VideoParameters& params,
                    std::string encoded_data,
                    std::string encoded_alpha,
                    base::TimeTicks timestamp,
                    bool keyframe));
  void OnEncodedVideo(const media::WebmMuxer::VideoParameters& params,
                      std::unique_ptr<std::string> encoded_data,
                      std::unique_ptr<std::string> encoded_alpha,
                      base::TimeTicks timestamp,
                      bool is_key_frame) {
    DoOnEncodedVideo(params, *encoded_data,
                     encoded_alpha ? *encoded_alpha : std::string(), timestamp,
                     is_key_frame);
  }

  void Encode(const scoped_refptr<VideoFrame>& frame,
              base::TimeTicks capture_time) {
    EXPECT_TRUE(message_loop_.IsCurrent());
    video_track_recorder_->OnVideoFrameForTesting(frame, capture_time);
  }

  bool CanEncodeAlphaChannel() {
    return video_track_recorder_->CanEncodeAlphaChannelForTesting();
  }

  // A ChildProcess and a MessageLoopForUI are both needed to fool the Tracks
  // and Sources below into believing they are on the right threads.
  const base::MessageLoopForUI message_loop_;
  const ChildProcess child_process_;

  // All members are non-const due to the series of initialize() calls needed.
  // |mock_source_| is owned by |blink_source_|, |track_| by |blink_track_|.
  MockMediaStreamVideoSource* mock_source_;
  blink::WebMediaStreamSource blink_source_;
  MediaStreamVideoTrack* track_;
  blink::WebMediaStreamTrack blink_track_;

  std::unique_ptr<VideoTrackRecorder> video_track_recorder_;

 private:
  DISALLOW_COPY_AND_ASSIGN(VideoTrackRecorderTest);
};

// Construct and destruct all objects, in particular |video_track_recorder_| and
// its inner object(s). This is a non trivial sequence.
TEST_P(VideoTrackRecorderTest, ConstructAndDestruct) {
  InitializeRecorder(testing::get<0>(GetParam()));
}

// Creates the encoder and encodes 2 frames of the same size; the encoder
// should be initialised and produce a keyframe, then a non-keyframe. Finally
// a frame of larger size is sent and is expected to be encoded as a keyframe.
// If |encode_alpha_channel| is enabled, encoder is expected to return a
// second output with encoded alpha data.
TEST_P(VideoTrackRecorderTest, VideoEncoding) {
  InitializeRecorder(testing::get<0>(GetParam()));

  const bool encode_alpha_channel = testing::get<2>(GetParam());
  // |frame_size| cannot be arbitrarily small, should be reasonable.
  const gfx::Size& frame_size = testing::get<1>(GetParam());
  const scoped_refptr<VideoFrame> video_frame =
      encode_alpha_channel ? VideoFrame::CreateTransparentFrame(frame_size)
                           : VideoFrame::CreateBlackFrame(frame_size);
  const double kFrameRate = 60.0f;
  video_frame->metadata()->SetDouble(media::VideoFrameMetadata::FRAME_RATE,
                                     kFrameRate);

  InSequence s;
  const base::TimeTicks timeticks_now = base::TimeTicks::Now();
  base::StringPiece first_frame_encoded_data;
  base::StringPiece first_frame_encoded_alpha;
  EXPECT_CALL(*this, DoOnEncodedVideo(_, _, _, timeticks_now, true))
      .Times(1)
      .WillOnce(DoAll(SaveArg<1>(&first_frame_encoded_data),
                      SaveArg<2>(&first_frame_encoded_alpha)));
  Encode(video_frame, timeticks_now);

  // Send another Video Frame.
  const base::TimeTicks timeticks_later = base::TimeTicks::Now();
  base::StringPiece second_frame_encoded_data;
  base::StringPiece second_frame_encoded_alpha;
  EXPECT_CALL(*this, DoOnEncodedVideo(_, _, _, timeticks_later, false))
      .Times(1)
      .WillOnce(DoAll(SaveArg<1>(&second_frame_encoded_data),
                      SaveArg<2>(&second_frame_encoded_alpha)));
  Encode(video_frame, timeticks_later);

  // Send another Video Frame and expect only an DoOnEncodedVideo() callback.
  const gfx::Size frame_size2(frame_size.width() + kTrackRecorderTestSizeDiff,
                              frame_size.height());
  const scoped_refptr<VideoFrame> video_frame2 =
      encode_alpha_channel ? VideoFrame::CreateTransparentFrame(frame_size2)
                           : VideoFrame::CreateBlackFrame(frame_size2);

  base::RunLoop run_loop;
  base::Closure quit_closure = run_loop.QuitClosure();

  base::StringPiece third_frame_encoded_data;
  base::StringPiece third_frame_encoded_alpha;
  EXPECT_CALL(*this, DoOnEncodedVideo(_, _, _, _, true))
      .Times(1)
      .WillOnce(DoAll(SaveArg<1>(&third_frame_encoded_data),
                      SaveArg<2>(&third_frame_encoded_alpha),
                      RunClosure(quit_closure)));
  Encode(video_frame2, base::TimeTicks::Now());

  run_loop.Run();

  const size_t kEncodedSizeThreshold = 14;
  EXPECT_GE(first_frame_encoded_data.size(), kEncodedSizeThreshold);
  EXPECT_GE(second_frame_encoded_data.size(), kEncodedSizeThreshold);
  EXPECT_GE(third_frame_encoded_data.size(), kEncodedSizeThreshold);

  if (encode_alpha_channel && CanEncodeAlphaChannel()) {
    EXPECT_GE(first_frame_encoded_alpha.size(), kEncodedSizeThreshold);
    EXPECT_GE(second_frame_encoded_alpha.size(), kEncodedSizeThreshold);
    EXPECT_GE(third_frame_encoded_alpha.size(), kEncodedSizeThreshold);
  } else {
    const size_t kEmptySize = 0;
    EXPECT_EQ(first_frame_encoded_alpha.size(), kEmptySize);
    EXPECT_EQ(second_frame_encoded_alpha.size(), kEmptySize);
    EXPECT_EQ(third_frame_encoded_alpha.size(), kEmptySize);
  }

  Mock::VerifyAndClearExpectations(this);
}

// Inserts an opaque frame followed by two transparent frames and expects the
// newly introduced transparent frame to force keyframe output.
TEST_F(VideoTrackRecorderTest, ForceKeyframeOnAlphaSwitch) {
  InitializeRecorder(VideoTrackRecorder::CodecId::VP8);

  const gfx::Size& frame_size = kTrackRecorderTestSize[0];
  const scoped_refptr<VideoFrame> opaque_frame =
      VideoFrame::CreateBlackFrame(frame_size);

  InSequence s;
  base::StringPiece first_frame_encoded_alpha;
  EXPECT_CALL(*this, DoOnEncodedVideo(_, _, _, _, true))
      .Times(1)
      .WillOnce(SaveArg<2>(&first_frame_encoded_alpha));
  Encode(opaque_frame, base::TimeTicks::Now());

  const scoped_refptr<VideoFrame> alpha_frame =
      VideoFrame::CreateTransparentFrame(frame_size);
  base::StringPiece second_frame_encoded_alpha;
  EXPECT_CALL(*this, DoOnEncodedVideo(_, _, _, _, true))
      .Times(1)
      .WillOnce(SaveArg<2>(&second_frame_encoded_alpha));
  Encode(alpha_frame, base::TimeTicks::Now());

  base::RunLoop run_loop;
  base::Closure quit_closure = run_loop.QuitClosure();
  base::StringPiece third_frame_encoded_alpha;
  EXPECT_CALL(*this, DoOnEncodedVideo(_, _, _, _, false))
      .Times(1)
      .WillOnce(DoAll(SaveArg<2>(&third_frame_encoded_alpha),
                      RunClosure(quit_closure)));
  Encode(alpha_frame, base::TimeTicks::Now());
  run_loop.Run();

  const size_t kEmptySize = 0;
  EXPECT_EQ(first_frame_encoded_alpha.size(), kEmptySize);
  EXPECT_GT(second_frame_encoded_alpha.size(), kEmptySize);
  EXPECT_GT(third_frame_encoded_alpha.size(), kEmptySize);

  Mock::VerifyAndClearExpectations(this);
}

INSTANTIATE_TEST_CASE_P(,
                        VideoTrackRecorderTest,
                        ::testing::Combine(ValuesIn(kTrackRecorderTestCodec),
                                           ValuesIn(kTrackRecorderTestSize),
                                           ::testing::Bool()));

}  // namespace content
