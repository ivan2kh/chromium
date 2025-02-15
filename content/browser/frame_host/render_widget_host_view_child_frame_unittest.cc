// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/frame_host/render_widget_host_view_child_frame.h"

#include <stdint.h>

#include <utility>

#include "base/macros.h"
#include "base/memory/ptr_util.h"
#include "base/message_loop/message_loop.h"
#include "base/run_loop.h"
#include "base/single_thread_task_runner.h"
#include "build/build_config.h"
#include "cc/surfaces/surface.h"
#include "cc/surfaces/surface_factory.h"
#include "cc/surfaces/surface_manager.h"
#include "cc/surfaces/surface_sequence.h"
#include "content/browser/compositor/test/no_transport_image_transport_factory.h"
#include "content/browser/frame_host/cross_process_frame_connector.h"
#include "content/browser/gpu/compositor_util.h"
#include "content/browser/renderer_host/render_widget_host_delegate.h"
#include "content/browser/renderer_host/render_widget_host_impl.h"
#include "content/common/view_messages.h"
#include "content/public/browser/render_widget_host_view.h"
#include "content/public/test/mock_render_process_host.h"
#include "content/public/test/test_browser_context.h"
#include "content/test/test_render_view_host.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/compositor/compositor.h"

namespace content {
namespace {
class MockRenderWidgetHostDelegate : public RenderWidgetHostDelegate {
 public:
  MockRenderWidgetHostDelegate() {}
  ~MockRenderWidgetHostDelegate() override {}
 private:
  void Cut() override {}
  void Copy() override {}
  void Paste() override {}
  void SelectAll() override {}
};

class MockCrossProcessFrameConnector : public CrossProcessFrameConnector {
 public:
  MockCrossProcessFrameConnector() : CrossProcessFrameConnector(nullptr) {}
  ~MockCrossProcessFrameConnector() override {}

  void SetChildFrameSurface(const cc::SurfaceInfo& surface_info,
                            const cc::SurfaceSequence& sequence) override {
    last_surface_info_ = surface_info;
  }

  RenderWidgetHostViewBase* GetParentRenderWidgetHostView() override {
    return nullptr;
  }

  cc::SurfaceInfo last_surface_info_;
};

}  // namespace

class RenderWidgetHostViewChildFrameTest : public testing::Test {
 public:
  RenderWidgetHostViewChildFrameTest() {}

  void SetUp() override {
    browser_context_.reset(new TestBrowserContext);

// ImageTransportFactory doesn't exist on Android.
#if !defined(OS_ANDROID)
    ImageTransportFactory::InitializeForUnitTests(
        base::WrapUnique(new NoTransportImageTransportFactory));
#endif

    MockRenderProcessHost* process_host =
        new MockRenderProcessHost(browser_context_.get());
    int32_t routing_id = process_host->GetNextRoutingID();
    widget_host_ =
        new RenderWidgetHostImpl(&delegate_, process_host, routing_id, false);
    view_ = RenderWidgetHostViewChildFrame::Create(widget_host_);

    test_frame_connector_ = new MockCrossProcessFrameConnector();
    view_->SetCrossProcessFrameConnector(test_frame_connector_);
  }

  void TearDown() override {
    if (view_)
      view_->Destroy();
    delete widget_host_;
    delete test_frame_connector_;

    browser_context_.reset();

    message_loop_.task_runner()->DeleteSoon(FROM_HERE,
                                            browser_context_.release());
    base::RunLoop().RunUntilIdle();
#if !defined(OS_ANDROID)
    ImageTransportFactory::Terminate();
#endif
  }

  cc::SurfaceId GetSurfaceId() const {
    return cc::SurfaceId(view_->frame_sink_id_, view_->local_surface_id_);
  }

  void ClearCompositorSurfaceIfNecessary() {
    view_->ClearCompositorSurfaceIfNecessary();
  }

 protected:
  base::MessageLoopForUI message_loop_;
  std::unique_ptr<BrowserContext> browser_context_;
  MockRenderWidgetHostDelegate delegate_;

  // Tests should set these to NULL if they've already triggered their
  // destruction.
  RenderWidgetHostImpl* widget_host_;
  RenderWidgetHostViewChildFrame* view_;
  MockCrossProcessFrameConnector* test_frame_connector_;

 private:
  DISALLOW_COPY_AND_ASSIGN(RenderWidgetHostViewChildFrameTest);
};

cc::CompositorFrame CreateDelegatedFrame(float scale_factor,
                                         gfx::Size size,
                                         const gfx::Rect& damage) {
  cc::CompositorFrame frame;
  frame.metadata.device_scale_factor = scale_factor;

  std::unique_ptr<cc::RenderPass> pass = cc::RenderPass::Create();
  pass->SetNew(1, gfx::Rect(size), damage, gfx::Transform());
  frame.render_pass_list.push_back(std::move(pass));
  return frame;
}

// http://crbug.com/696919
#if defined(OS_WIN)
#define MAYBE_VisibilityTest DISABLED_VisibilityTest
#define MAYBE_SwapCompositorFrame DISABLED_SwapCompositorFrame
#else
#define MAYBE_VisibilityTest VisibilityTest
#define MAYBE_SwapCompositorFrame SwapCompositorFrame
#endif

TEST_F(RenderWidgetHostViewChildFrameTest, MAYBE_VisibilityTest) {
  view_->Show();
  ASSERT_TRUE(view_->IsShowing());

  view_->Hide();
  ASSERT_FALSE(view_->IsShowing());
}

// Verify that OnSwapCompositorFrame behavior is correct when a delegated
// frame is received from a renderer process.
TEST_F(RenderWidgetHostViewChildFrameTest, MAYBE_SwapCompositorFrame) {
  gfx::Size view_size(100, 100);
  gfx::Rect view_rect(view_size);
  float scale_factor = 1.f;

  view_->SetSize(view_size);
  view_->Show();

  view_->OnSwapCompositorFrame(
      0, CreateDelegatedFrame(scale_factor, view_size, view_rect));

  cc::SurfaceId id = GetSurfaceId();
  if (id.is_valid()) {
#if !defined(OS_ANDROID)
    ImageTransportFactory* factory = ImageTransportFactory::GetInstance();
    cc::SurfaceManager* manager =
        factory->GetContextFactoryPrivate()->GetSurfaceManager();
    cc::Surface* surface = manager->GetSurfaceForId(id);
    EXPECT_TRUE(surface);
    // There should be a SurfaceSequence created by the RWHVChildFrame.
    EXPECT_EQ(1u, surface->GetDestructionDependencyCount());
#endif

    // Surface ID should have been passed to CrossProcessFrameConnector to
    // be sent to the embedding renderer.
    EXPECT_EQ(cc::SurfaceInfo(id, scale_factor, view_size),
              test_frame_connector_->last_surface_info_);
  }
}

// Check that frame eviction does not trigger allocation of a new local surface
// id.
TEST_F(RenderWidgetHostViewChildFrameTest, FrameEvictionKeepsLocalSurfaceId) {
  gfx::Size view_size(100, 100);
  gfx::Rect view_rect(view_size);
  float scale_factor = 1.f;

  view_->SetSize(view_size);
  view_->Show();

  // Submit a frame. Remember the local surface id and check that has_frame()
  // returns true.
  view_->OnSwapCompositorFrame(
      0, CreateDelegatedFrame(scale_factor, view_size, view_rect));

  cc::SurfaceId surface_id = GetSurfaceId();
  EXPECT_TRUE(surface_id.is_valid());
  EXPECT_TRUE(view_->has_frame());

  // Evict the frame. The surface id must remain the same but has_frame() should
  // return false.
  ClearCompositorSurfaceIfNecessary();
  EXPECT_EQ(surface_id, GetSurfaceId());
  EXPECT_FALSE(view_->has_frame());

  // Submit another frame. Since it has the same size and scale as the first
  // one, the same surface id must be used. has_frame() must return true.
  view_->OnSwapCompositorFrame(
      0, CreateDelegatedFrame(scale_factor, view_size, view_rect));
  EXPECT_EQ(surface_id, GetSurfaceId());
  EXPECT_TRUE(view_->has_frame());
}

}  // namespace content
