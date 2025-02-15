/*
 * Copyright (C) 2006 Apple Computer, Inc.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "platform/image-decoders/gif/GIFImageDecoder.h"

#include "platform/image-decoders/gif/GIFImageReader.h"
#include "wtf/NotFound.h"
#include "wtf/PtrUtil.h"
#include <limits>

namespace blink {

GIFImageDecoder::GIFImageDecoder(AlphaOption alphaOption,
                                 const ColorBehavior& colorBehavior,
                                 size_t maxDecodedBytes)
    : ImageDecoder(alphaOption, colorBehavior, maxDecodedBytes),
      m_repetitionCount(cAnimationLoopOnce) {}

GIFImageDecoder::~GIFImageDecoder() {}

void GIFImageDecoder::onSetData(SegmentReader* data) {
  if (m_reader)
    m_reader->setData(data);
}

int GIFImageDecoder::repetitionCount() const {
  // This value can arrive at any point in the image data stream.  Most GIFs
  // in the wild declare it near the beginning of the file, so it usually is
  // set by the time we've decoded the size, but (depending on the GIF and the
  // packets sent back by the webserver) not always.  If the reader hasn't
  // seen a loop count yet, it will return cLoopCountNotSeen, in which case we
  // should default to looping once (the initial value for
  // |m_repetitionCount|).
  //
  // There are some additional wrinkles here. First, ImageSource::clear()
  // may destroy the reader, making the result from the reader _less_
  // authoritative on future calls if the recreated reader hasn't seen the
  // loop count.  We don't need to special-case this because in this case the
  // new reader will once again return cLoopCountNotSeen, and we won't
  // overwrite the cached correct value.
  //
  // Second, a GIF might never set a loop count at all, in which case we
  // should continue to treat it as a "loop once" animation.  We don't need
  // special code here either, because in this case we'll never change
  // |m_repetitionCount| from its default value.
  //
  // Third, we use the same GIFImageReader for counting frames and we might
  // see the loop count and then encounter a decoding error which happens
  // later in the stream. It is also possible that no frames are in the
  // stream. In these cases we should just loop once.
  if (isAllDataReceived() && parseCompleted() && m_reader->imagesCount() == 1)
    m_repetitionCount = cAnimationNone;
  else if (failed() || (m_reader && (!m_reader->imagesCount())))
    m_repetitionCount = cAnimationLoopOnce;
  else if (m_reader && m_reader->loopCount() != cLoopCountNotSeen)
    m_repetitionCount = m_reader->loopCount();
  return m_repetitionCount;
}

bool GIFImageDecoder::frameIsCompleteAtIndex(size_t index) const {
  return m_reader && (index < m_reader->imagesCount()) &&
         m_reader->frameContext(index)->isComplete();
}

float GIFImageDecoder::frameDurationAtIndex(size_t index) const {
  return (m_reader && (index < m_reader->imagesCount()) &&
          m_reader->frameContext(index)->isHeaderDefined())
             ? m_reader->frameContext(index)->delayTime()
             : 0;
}

bool GIFImageDecoder::setFailed() {
  m_reader.reset();
  return ImageDecoder::setFailed();
}

bool GIFImageDecoder::haveDecodedRow(size_t frameIndex,
                                     GIFRow::const_iterator rowBegin,
                                     size_t width,
                                     size_t rowNumber,
                                     unsigned repeatCount,
                                     bool writeTransparentPixels) {
  const GIFFrameContext* frameContext = m_reader->frameContext(frameIndex);
  // The pixel data and coordinates supplied to us are relative to the frame's
  // origin within the entire image size, i.e.
  // (frameContext->xOffset, frameContext->yOffset). There is no guarantee
  // that width == (size().width() - frameContext->xOffset), so
  // we must ensure we don't run off the end of either the source data or the
  // row's X-coordinates.
  const int xBegin = frameContext->xOffset();
  const int yBegin = frameContext->yOffset() + rowNumber;
  const int xEnd = std::min(static_cast<int>(frameContext->xOffset() + width),
                            size().width());
  const int yEnd = std::min(
      static_cast<int>(frameContext->yOffset() + rowNumber + repeatCount),
      size().height());
  if (!width || (xBegin < 0) || (yBegin < 0) || (xEnd <= xBegin) ||
      (yEnd <= yBegin))
    return true;

  const GIFColorMap::Table& colorTable =
      frameContext->localColorMap().isDefined()
          ? frameContext->localColorMap().getTable()
          : m_reader->globalColorMap().getTable();

  if (colorTable.isEmpty())
    return true;

  GIFColorMap::Table::const_iterator colorTableIter = colorTable.begin();

  // Initialize the frame if necessary.
  ImageFrame& buffer = m_frameBufferCache[frameIndex];
  if (!initFrameBuffer(frameIndex))
    return false;

  const size_t transparentPixel = frameContext->transparentPixel();
  GIFRow::const_iterator rowEnd = rowBegin + (xEnd - xBegin);
  ImageFrame::PixelData* currentAddress = buffer.getAddr(xBegin, yBegin);

  // We may or may not need to write transparent pixels to the buffer.
  // If we're compositing against a previous image, it's wrong, and if
  // we're writing atop a cleared, fully transparent buffer, it's
  // unnecessary; but if we're decoding an interlaced gif and
  // displaying it "Haeberli"-style, we must write these for passes
  // beyond the first, or the initial passes will "show through" the
  // later ones.
  //
  // The loops below are almost identical. One writes a transparent pixel
  // and one doesn't based on the value of |writeTransparentPixels|.
  // The condition check is taken out of the loop to enhance performance.
  // This optimization reduces decoding time by about 15% for a 3MB image.
  if (writeTransparentPixels) {
    for (; rowBegin != rowEnd; ++rowBegin, ++currentAddress) {
      const size_t sourceValue = *rowBegin;
      if ((sourceValue != transparentPixel) &&
          (sourceValue < colorTable.size())) {
        *currentAddress = colorTableIter[sourceValue];
      } else {
        *currentAddress = 0;
        m_currentBufferSawAlpha = true;
      }
    }
  } else {
    for (; rowBegin != rowEnd; ++rowBegin, ++currentAddress) {
      const size_t sourceValue = *rowBegin;
      if ((sourceValue != transparentPixel) &&
          (sourceValue < colorTable.size()))
        *currentAddress = colorTableIter[sourceValue];
      else
        m_currentBufferSawAlpha = true;
    }
  }

  // Tell the frame to copy the row data if need be.
  if (repeatCount > 1)
    buffer.copyRowNTimes(xBegin, xEnd, yBegin, yEnd);

  buffer.setPixelsChanged(true);
  return true;
}

bool GIFImageDecoder::parseCompleted() const {
  return m_reader && m_reader->parseCompleted();
}

bool GIFImageDecoder::frameComplete(size_t frameIndex) {
  // Initialize the frame if necessary.  Some GIFs insert do-nothing frames,
  // in which case we never reach haveDecodedRow() before getting here.
  if (!initFrameBuffer(frameIndex))
    return setFailed();

  if (!m_currentBufferSawAlpha)
    correctAlphaWhenFrameBufferSawNoAlpha(frameIndex);

  m_frameBufferCache[frameIndex].setStatus(ImageFrame::FrameComplete);

  return true;
}

void GIFImageDecoder::clearFrameBuffer(size_t frameIndex) {
  if (m_reader &&
      m_frameBufferCache[frameIndex].getStatus() == ImageFrame::FramePartial) {
    // Reset the state of the partial frame in the reader so that the frame
    // can be decoded again when requested.
    m_reader->clearDecodeState(frameIndex);
  }
  ImageDecoder::clearFrameBuffer(frameIndex);
}

size_t GIFImageDecoder::decodeFrameCount() {
  parse(GIFFrameCountQuery);
  // If decoding fails, |m_reader| will have been destroyed.  Instead of
  // returning 0 in this case, return the existing number of frames.  This way
  // if we get halfway through the image before decoding fails, we won't
  // suddenly start reporting that the image has zero frames.
  return failed() ? m_frameBufferCache.size() : m_reader->imagesCount();
}

void GIFImageDecoder::initializeNewFrame(size_t index) {
  ImageFrame* buffer = &m_frameBufferCache[index];
  const GIFFrameContext* frameContext = m_reader->frameContext(index);
  buffer->setOriginalFrameRect(
      intersection(frameContext->frameRect(), IntRect(IntPoint(), size())));
  buffer->setDuration(frameContext->delayTime());
  buffer->setDisposalMethod(frameContext->getDisposalMethod());
  buffer->setRequiredPreviousFrameIndex(
      findRequiredPreviousFrame(index, false));
}

void GIFImageDecoder::decode(size_t index) {
  parse(GIFFrameCountQuery);

  if (failed())
    return;

  updateAggressivePurging(index);

  Vector<size_t> framesToDecode = findFramesToDecode(index);
  for (auto i = framesToDecode.rbegin(); i != framesToDecode.rend(); ++i) {
    if (!m_reader->decode(*i)) {
      setFailed();
      return;
    }

    // If this returns false, we need more data to continue decoding.
    if (!postDecodeProcessing(*i))
      break;
  }

  // It is also a fatal error if all data is received and we have decoded all
  // frames available but the file is truncated.
  if (index >= m_frameBufferCache.size() - 1 && isAllDataReceived() &&
      m_reader && !m_reader->parseCompleted())
    setFailed();
}

void GIFImageDecoder::parse(GIFParseQuery query) {
  if (failed())
    return;

  if (!m_reader) {
    m_reader = WTF::makeUnique<GIFImageReader>(this);
    m_reader->setData(m_data);
  }

  if (!m_reader->parse(query))
    setFailed();
}

void GIFImageDecoder::onInitFrameBuffer(size_t frameIndex) {
  m_currentBufferSawAlpha = false;
}

bool GIFImageDecoder::canReusePreviousFrameBuffer(size_t frameIndex) const {
  DCHECK(frameIndex < m_frameBufferCache.size());
  return m_frameBufferCache[frameIndex].getDisposalMethod() !=
         ImageFrame::DisposeOverwritePrevious;
}

}  // namespace blink
