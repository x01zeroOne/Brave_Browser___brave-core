/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_FRAME_VERTICAL_TAB_STRIP_WIDGET_DELEGATE_VIEW_H_
#define BRAVE_BROWSER_UI_VIEWS_FRAME_VERTICAL_TAB_STRIP_WIDGET_DELEGATE_VIEW_H_

#include <memory>

#include "brave/browser/ui/views/frame/vertical_tab_strip_region_view.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "ui/views/layout/fill_layout.h"
#include "ui/views/view_observer.h"
#include "ui/views/widget/widget_delegate.h"
#include "ui/views/widget/widget_observer.h"

class BrowserView;
class VerticalTabStripRegionView;

// This class wraps VerticalTabStripRegionView and show them atop a Widget.
// Vertical tab strip could be overlaps with contents web view and
// we need a Widget to accept user events ahead of contents web view.
// This Widget's coordinates and visibility are synchronized with a host view
// given by Create() method. The client of this class should attach this
// to parent widget, typically BrowserView/Frame. An then this widget will be a
// child of BrowserView's Widget with Control widget type.
//
// Usage:
//  auto* host_view = AddChildView(std::make_unique<views::View>());
//  VerticalTabStripWidgetDelegateView::Create(browser_view, host_view);
//  host_view->SetVisible(true);  // will show up the widget
//  host_view->SetBounds(gfx::Rect(0, 0, 100, 100));  // will layout the widget.
//                                                    // But size could be
//                                                    // different based on
//                                                    // state.
//

template <class BaseView>
class VerticalTabStripAdapterViewT : public BaseView,
                                     public views::ViewObserver {
 public:
  static_assert(std::is_base_of_v<views::View, BaseView>);

  ~VerticalTabStripAdapterViewT() override {
    // Child views will be deleted after this. Marks `region_view_` nullptr
    // so that they dont' access the `region_view_` via this view.
    region_view_ = nullptr;
  }

  VerticalTabStripRegionView* vertical_tab_strip_region_view() const {
    return region_view_;
  }

 protected:
  FRIEND_TEST_ALL_PREFIXES(VerticalTabStripBrowserTest, VisualState);

  VerticalTabStripAdapterViewT(BrowserView* browser_view, views::View* host)
      : browser_view_(browser_view),
        host_(host),
        region_view_(
            BaseView::AddChildView(std::make_unique<VerticalTabStripRegionView>(
                browser_view_,
                browser_view_->tab_strip_region_view()))) {
    BaseView::SetLayoutManager(std::make_unique<views::FillLayout>());

    host_view_observation_.Observe(host_);

    ChildPreferredSizeChanged(region_view_);
  }

  virtual void UpdateAdapterViewBounds() = 0;

  // BaseView:
  void ChildPreferredSizeChanged(views::View* child) override {
    if (!host_) {
      return;
    }

    // Setting minimum size for |host_| so that we can overlay vertical tabs
    // over the web view.
    auto new_host_size = region_view_->GetMinimumSize();
    if (new_host_size != host_->GetPreferredSize()) {
      host_->SetPreferredSize(new_host_size);
      return;
    }

    // Layout adapter view bounds manually because host won't trigger layout.
    UpdateAdapterViewBounds();
  }

  void OnViewBoundsChanged(views::View* observed_view) override {
    UpdateAdapterViewBounds();
  }

  void OnViewIsDeleting(views::View* observed_view) override {
    host_view_observation_.Reset();
    host_ = nullptr;
  }

  raw_ptr<BrowserView> browser_view_ = nullptr;
  raw_ptr<views::View> host_ = nullptr;
  raw_ptr<VerticalTabStripRegionView> region_view_ = nullptr;

  base::ScopedObservation<views::View, views::ViewObserver>
      host_view_observation_{this};
};

#if BUILDFLAG(IS_MAC)
class VerticalTabStripAdapterView
    : public VerticalTabStripAdapterViewT<views::WidgetDelegateView>,
      public views::WidgetObserver {
 public:
  METADATA_HEADER(VerticalTabStripAdapterView);

  static constexpr bool kBackedByWidget = true;

  static VerticalTabStripAdapterView* Create(BrowserView* browser_view,
                                             views::View* host_view);
  ~VerticalTabStripAdapterView() override;

  // VerticalTabStripAdapterViewT:
  void UpdateAdapterViewBounds() override;
  void OnViewVisibilityChanged(views::View* observed_view,
                               views::View* starting_view) override;

  // views::WidgetObserver:
  void OnWidgetBoundsChanged(views::Widget* widget,
                             const gfx::Rect& new_bounds) override;
  void OnWidgetDestroying(views::Widget* widget) override;

 private:
  VerticalTabStripAdapterView(BrowserView* browser_view, views::View* host);

  void UpdateClip();

  base::ScopedObservation<views::Widget, views::WidgetObserver>
      widget_observation_{this};
};
#else
class VerticalTabStripAdapterView : public VerticalTabStripAdapterViewT<views::View> {
 public:
  METADATA_HEADER(VerticalTabStripAdapterView);

  static constexpr bool kBackedByWidget = false;

  static VerticalTabStripAdapterView* Create(BrowserView* browser_view,
                                             views::View* host_view);
  ~VerticalTabStripAdapterView() override;

  // VerticalTabStripAdapterViewT:
  void UpdateAdapterViewBounds() override;

 private:
  VerticalTabStripAdapterView(BrowserView* browser_view, views::View* host);
};
#endif

#endif  // BRAVE_BROWSER_UI_VIEWS_FRAME_VERTICAL_TAB_STRIP_WIDGET_DELEGATE_VIEW_H_
