/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/frame/vertical_tab_strip_widget_delegate_view.h"

#include <utility>

#include "base/check.h"
#include "brave/browser/ui/views/frame/brave_browser_view.h"
#include "chrome/browser/ui/views/theme_copying_widget.h"
#include "chrome/common/pref_names.h"
#include "ui/base/metadata/metadata_impl_macros.h"

#if defined(USE_AURA)
#include "ui/views/view_constants_aura.h"
#endif

VerticalTabStripAdapterView::~VerticalTabStripAdapterView() = default;

#if BUILDFLAG(IS_MAC)
// static
VerticalTabStripAdapterView* VerticalTabStripAdapterView::Create(
    BrowserView* browser_view,
    views::View* host_view) {
  DCHECK(browser_view->GetWidget());

  auto* delegate_view =
      new VerticalTabStripAdapterView(browser_view, host_view);
  views::Widget::InitParams params;
  params.delegate = delegate_view;

  params.parent = browser_view->GetWidget()->GetNativeView();
  params.type = views::Widget::InitParams::TYPE_CONTROL;
  // We need this to pass the key events to the top level widget. i.e. we should
  // not get focus.
  params.activatable = views::Widget::InitParams::Activatable::kNo;

  auto widget = std::make_unique<ThemeCopyingWidget>(browser_view->GetWidget());
  widget->Init(std::move(params));
  widget->Show();
  widget.release();

  return delegate_view;
}

VerticalTabStripAdapterView::VerticalTabStripAdapterView(
    BrowserView* browser_view,
    views::View* host)
    : VerticalTabStripAdapterViewT(browser_view, host) {
    widget_observation_.Observe(host_->GetWidget());
}

void VerticalTabStripAdapterView::OnViewVisibilityChanged(
    views::View* observed_view,
    views::View* starting_view) {
  auto* widget = GetWidget();
  if (!widget || widget->IsVisible() == observed_view->GetVisible())
    return;

  if (observed_view->GetVisible())
    widget->Show();
  else
    widget->Hide();
}

void VerticalTabStripAdapterView::OnWidgetBoundsChanged(
    views::Widget* widget,
    const gfx::Rect& new_bounds) {
  // The parent widget could be resized because fullscreen status changed.
  // Try resetting preferred size.
  ChildPreferredSizeChanged(region_view_);
}

void VerticalTabStripAdapterView::UpdateAdapterViewBounds() {
  if (!host_)
    return;

  auto* widget = GetWidget();
  if (!widget)
    return;

  // Convert coordinate system based on Browser's widget.
  gfx::Rect widget_bounds = host_->ConvertRectToWidget(host_->GetLocalBounds());
  widget_bounds.set_width(region_view_->GetPreferredSize().width());
  if (widget_bounds.IsEmpty()) {
    widget->Hide();
    return;
  }

  if (!widget->IsVisible())
    widget->Show();

  const bool need_to_call_layout =
      widget->GetWindowBoundsInScreen().size() != widget_bounds.size();
  widget->SetBounds(widget_bounds);

  if (auto insets = host_->GetInsets(); GetInsets() != insets)
    SetBorder(insets.IsEmpty() ? nullptr : views::CreateEmptyBorder(insets));

  if (need_to_call_layout)
    Layout();

  UpdateClip();
}

void VerticalTabStripAdapterView::OnWidgetDestroying(views::Widget* widget) {
  widget_observation_.Reset();
}

void VerticalTabStripAdapterView::UpdateClip() {
  // On mac, child window can be drawn out of parent window. We should clip
  // the border line and corner radius manually.
  // The corner radius value refers to the that of menu widget. Looks fit for
  // us.
  // https://github.com/chromium/chromium/blob/371d67fd9c7db16c32f22e3ba247a07aa5e81487/ui/views/controls/menu/menu_config_mac.mm#L35
  SkPath path;
  constexpr int kCornerRadius = 8;
  path.moveTo(0, 0);
  path.lineTo(width(), 0);
  path.lineTo(width(), height() - 1);
  path.lineTo(0 + kCornerRadius, height() - 1);
  path.rArcTo(kCornerRadius, kCornerRadius, 0, SkPath::kSmall_ArcSize,
              SkPathDirection::kCW, -kCornerRadius, -kCornerRadius);
  path.close();
  SetClipPath(path);
}

BEGIN_METADATA(VerticalTabStripAdapterView, views::WidgetDelegateView)
END_METADATA
#else
// static
VerticalTabStripAdapterView* VerticalTabStripAdapterView::Create(
    BrowserView* browser_view,
    views::View* host_view) {
  auto* delegate_view =
      new VerticalTabStripAdapterView(browser_view, host_view);

  DCHECK(host_view);
  host_view->AddChildView(delegate_view);

  return delegate_view;
}

VerticalTabStripAdapterView::VerticalTabStripAdapterView(
    BrowserView* browser_view,
    views::View* host)
    : VerticalTabStripAdapterViewT(browser_view, host) {
  SetPaintToLayer();
}

void VerticalTabStripAdapterView::UpdateAdapterViewBounds() {
  if (!host_)
    return;

  gfx::Rect bounds = host_->GetLocalBounds();
  bounds.set_width(region_view_->GetPreferredSize().width());
  SetBoundsRect(bounds);

  if (auto insets = host_->GetInsets(); GetInsets() != insets)
    SetBorder(insets.IsEmpty() ? nullptr : views::CreateEmptyBorder(insets));

  Layout();
}

BEGIN_METADATA(VerticalTabStripAdapterView, views::View)
END_METADATA
#endif
