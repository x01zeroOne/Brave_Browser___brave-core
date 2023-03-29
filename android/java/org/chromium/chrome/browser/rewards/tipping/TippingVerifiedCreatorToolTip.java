/**
 * Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/.
 */

package org.chromium.chrome.browser.rewards.tipping;

import android.annotation.SuppressLint;
import android.content.Context;
import android.content.res.Resources;
import android.util.TypedValue;
import android.view.Gravity;
import android.view.LayoutInflater;
import android.view.MotionEvent;
import android.view.View;
import android.widget.PopupWindow;

import androidx.annotation.NonNull;
import androidx.appcompat.content.res.AppCompatResources;

import org.chromium.chrome.R;

public class TippingVerifiedCreatorToolTip {
    private PopupWindow mPopupWindow;
    public TippingVerifiedCreatorToolTip(@NonNull Context context) {
        init(context);
    }

    @SuppressLint("ClickableViewAccessibility")
    private void init(Context context) {
        LayoutInflater inflater =
                (LayoutInflater) context.getSystemService(Context.LAYOUT_INFLATER_SERVICE);
        View contentView = inflater.inflate(R.layout.tipping_verified_creator_tooltip, null, false);

        mPopupWindow = new PopupWindow(context);
        mPopupWindow.setContentView(contentView);
        mPopupWindow.setTouchable(true);
        mPopupWindow.setFocusable(true);
        mPopupWindow.setBackgroundDrawable(AppCompatResources.getDrawable(
                context, R.drawable.tipping_verified_creator_tooltip_background));
        mPopupWindow.setElevation(60);
        mPopupWindow.setWidth((int) convertDPtoPX(context, 280.0f));
        mPopupWindow.setTouchInterceptor((v, event) -> {
            if (event.getAction() == MotionEvent.ACTION_OUTSIDE) {
                mPopupWindow.dismiss();
                return true;
            }
            return false;
        });
    }

    private float convertDPtoPX(Context context, float dip) {
        Resources resources = context.getResources();
        return TypedValue.applyDimension(
                TypedValue.COMPLEX_UNIT_DIP, dip, resources.getDisplayMetrics());
    }

    public void show(@NonNull View anchorView) {
        int[] location = new int[2];
        anchorView.getLocationInWindow(location);
        mPopupWindow.showAtLocation(
                anchorView, Gravity.CENTER_HORIZONTAL | Gravity.TOP, 0, location[1] + 80);
    }
}
