// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/paint/VideoPainter.h"

#include "core/frame/FrameView.h"
#include "core/frame/Settings.h"
#include "core/html/HTMLMediaElement.h"
#include "core/loader/EmptyClients.h"
#include "core/paint/StubChromeClientForSPv2.h"
#include "core/testing/DummyPageHolder.h"
#include "platform/testing/EmptyWebMediaPlayer.h"
#include "platform/testing/RuntimeEnabledFeaturesTestHelpers.h"
#include "platform/testing/UnitTestHelpers.h"
#include "public/platform/Platform.h"
#include "public/platform/WebCompositorSupport.h"
#include "public/platform/WebLayer.h"
#include "public/platform/WebSize.h"
#include "testing/gtest/include/gtest/gtest.h"

// Integration tests of video painting code (in SPv2 mode).

namespace blink {
namespace {

class StubWebMediaPlayer : public EmptyWebMediaPlayer {
 public:
  StubWebMediaPlayer(WebMediaPlayerClient* client) : m_client(client) {}

  const WebLayer* getWebLayer() { return m_webLayer.get(); }

  // WebMediaPlayer
  void load(LoadType, const WebMediaPlayerSource&, CORSMode) {
    m_networkState = NetworkStateLoaded;
    m_client->networkStateChanged();
    m_readyState = ReadyStateHaveEnoughData;
    m_client->readyStateChanged();
    m_webLayer = Platform::current()->compositorSupport()->createLayer();
    m_client->setWebLayer(m_webLayer.get());
  }
  NetworkState getNetworkState() const override { return m_networkState; }
  ReadyState getReadyState() const override { return m_readyState; }

 private:
  WebMediaPlayerClient* m_client;
  std::unique_ptr<WebLayer> m_webLayer;
  NetworkState m_networkState = NetworkStateEmpty;
  ReadyState m_readyState = ReadyStateHaveNothing;
};

class StubLocalFrameClient : public EmptyLocalFrameClient {
 public:
  // LocalFrameClient
  std::unique_ptr<WebMediaPlayer> createWebMediaPlayer(
      HTMLMediaElement&,
      const WebMediaPlayerSource&,
      WebMediaPlayerClient* client) override {
    return WTF::makeUnique<StubWebMediaPlayer>(client);
  }
};

class VideoPainterTestForSPv2 : public ::testing::Test,
                                private ScopedSlimmingPaintV2ForTest {
 public:
  VideoPainterTestForSPv2() : ScopedSlimmingPaintV2ForTest(true) {}

 protected:
  void SetUp() override {
    m_chromeClient = new StubChromeClientForSPv2();
    m_localFrameClient = new StubLocalFrameClient;
    Page::PageClients clients;
    fillWithEmptyClients(clients);
    clients.chromeClient = m_chromeClient.get();
    m_pageHolder = DummyPageHolder::create(
        IntSize(800, 600), &clients, m_localFrameClient.get(),
        [](Settings& settings) {
          settings.setAcceleratedCompositingEnabled(true);
        });
    document().view()->setParentVisible(true);
    document().view()->setSelfVisible(true);
    document().setURL(KURL(KURL(), "https://example.com/"));
  }

  Document& document() { return m_pageHolder->document(); }
  bool hasLayerAttached(const WebLayer& layer) {
    return m_chromeClient->hasLayer(layer);
  }

 private:
  Persistent<StubChromeClientForSPv2> m_chromeClient;
  Persistent<StubLocalFrameClient> m_localFrameClient;
  std::unique_ptr<DummyPageHolder> m_pageHolder;
};

TEST_F(VideoPainterTestForSPv2, VideoLayerAppearsInLayerTree) {
  // Insert a <video> and allow it to begin loading.
  document().body()->setInnerHTML("<video width=300 height=200 src=test.ogv>");
  testing::runPendingTasks();

  // Force the page to paint.
  document().view()->updateAllLifecyclePhases();

  // Fetch the layer associated with the <video>, and check that it was
  // correctly configured in the layer tree.
  HTMLMediaElement* element =
      toHTMLMediaElement(document().body()->firstChild());
  StubWebMediaPlayer* player =
      static_cast<StubWebMediaPlayer*>(element->webMediaPlayer());
  const WebLayer* layer = player->getWebLayer();
  ASSERT_TRUE(layer);
  EXPECT_TRUE(hasLayerAttached(*layer));
  EXPECT_EQ(WebSize(300, 200), layer->bounds());
}

}  // namespace
}  // namespace blink
