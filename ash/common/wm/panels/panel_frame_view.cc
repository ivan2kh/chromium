// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/common/wm/panels/panel_frame_view.h"

#include "ash/common/frame/caption_buttons/frame_caption_button_container_view.h"
#include "ash/common/frame/default_header_painter.h"
#include "ash/common/frame/frame_border_hit_test.h"
#include "ash/common/wm_window.h"
#include "ash/shell.h"
#include "ui/aura/client/aura_constants.h"
#include "ui/aura/window.h"
#include "ui/base/hit_test.h"
#include "ui/gfx/canvas.h"
#include "ui/views/controls/image_view.h"
#include "ui/views/widget/widget.h"
#include "ui/views/widget/widget_delegate.h"

namespace ash {

// static
const char PanelFrameView::kViewClassName[] = "PanelFrameView";

PanelFrameView::PanelFrameView(views::Widget* frame, FrameType frame_type)
    : frame_(frame), caption_button_container_(nullptr), window_icon_(nullptr) {
  GetWidgetWindow()->InstallResizeHandleWindowTargeter(nullptr);
  DCHECK(!frame_->widget_delegate()->CanMaximize());
  if (frame_type != FRAME_NONE)
    InitHeaderPainter();
  Shell::GetInstance()->AddShellObserver(this);
}

PanelFrameView::~PanelFrameView() {
  Shell::GetInstance()->RemoveShellObserver(this);
}

void PanelFrameView::SetFrameColors(SkColor active_frame_color,
                                    SkColor inactive_frame_color) {
  header_painter_->SetFrameColors(active_frame_color, inactive_frame_color);
  GetWidgetWindow()->aura_window()->SetProperty(
      aura::client::kTopViewColor, header_painter_->GetInactiveFrameColor());
}

const char* PanelFrameView::GetClassName() const {
  return kViewClassName;
}

void PanelFrameView::InitHeaderPainter() {
  header_painter_.reset(new DefaultHeaderPainter);
  GetWidgetWindow()->aura_window()->SetProperty(
      aura::client::kTopViewColor, header_painter_->GetInactiveFrameColor());

  caption_button_container_ = new FrameCaptionButtonContainerView(frame_);
  AddChildView(caption_button_container_);

  header_painter_->Init(frame_, this, caption_button_container_);

  if (frame_->widget_delegate()->ShouldShowWindowIcon()) {
    window_icon_ = new views::ImageView();
    AddChildView(window_icon_);
    header_painter_->UpdateLeftHeaderView(window_icon_);
  }
}

WmWindow* PanelFrameView::GetWidgetWindow() {
  return WmWindow::Get(frame_->GetNativeWindow());
}

int PanelFrameView::NonClientTopBorderHeight() const {
  if (!header_painter_)
    return 0;
  return header_painter_->GetHeaderHeightForPainting();
}

gfx::Size PanelFrameView::GetMinimumSize() const {
  if (!header_painter_)
    return gfx::Size();
  gfx::Size min_client_view_size(frame_->client_view()->GetMinimumSize());
  return gfx::Size(std::max(header_painter_->GetMinimumHeaderWidth(),
                            min_client_view_size.width()),
                   NonClientTopBorderHeight() + min_client_view_size.height());
}

void PanelFrameView::Layout() {
  if (!header_painter_)
    return;
  header_painter_->LayoutHeader();
  GetWidgetWindow()->aura_window()->SetProperty(aura::client::kTopViewInset,
                                                NonClientTopBorderHeight());
}

void PanelFrameView::GetWindowMask(const gfx::Size&, gfx::Path*) {
  // Nothing.
}

void PanelFrameView::ResetWindowControls() {
  NOTIMPLEMENTED();
}

void PanelFrameView::UpdateWindowIcon() {
  if (!window_icon_)
    return;
  views::WidgetDelegate* delegate = frame_->widget_delegate();
  if (delegate)
    window_icon_->SetImage(delegate->GetWindowIcon());
  window_icon_->SchedulePaint();
}

void PanelFrameView::UpdateWindowTitle() {
  if (!header_painter_)
    return;
  header_painter_->SchedulePaintForTitle();
}

void PanelFrameView::SizeConstraintsChanged() {}

gfx::Rect PanelFrameView::GetBoundsForClientView() const {
  gfx::Rect client_bounds = bounds();
  client_bounds.Inset(0, NonClientTopBorderHeight(), 0, 0);
  return client_bounds;
}

gfx::Rect PanelFrameView::GetWindowBoundsForClientBounds(
    const gfx::Rect& client_bounds) const {
  gfx::Rect window_bounds = client_bounds;
  window_bounds.Inset(0, -NonClientTopBorderHeight(), 0, 0);
  return window_bounds;
}

int PanelFrameView::NonClientHitTest(const gfx::Point& point) {
  if (!header_painter_)
    return HTNOWHERE;
  return FrameBorderNonClientHitTest(this, caption_button_container_, point);
}

void PanelFrameView::OnPaint(gfx::Canvas* canvas) {
  if (!header_painter_)
    return;
  bool paint_as_active = ShouldPaintAsActive();
  caption_button_container_->SetPaintAsActive(paint_as_active);

  HeaderPainter::Mode header_mode = paint_as_active
                                        ? HeaderPainter::MODE_ACTIVE
                                        : HeaderPainter::MODE_INACTIVE;
  header_painter_->PaintHeader(canvas, header_mode);
}

///////////////////////////////////////////////////////////////////////////////
// PanelFrameView, ShellObserver overrides:

void PanelFrameView::OnOverviewModeStarting() {
  caption_button_container_->SetVisible(false);
}

void PanelFrameView::OnOverviewModeEnded() {
  caption_button_container_->SetVisible(true);
}

}  // namespace ash
