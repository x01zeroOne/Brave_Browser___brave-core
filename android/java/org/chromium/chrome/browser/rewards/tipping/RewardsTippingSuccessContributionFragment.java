/**
 * Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/.
 */

package org.chromium.chrome.browser.rewards.tipping;

import android.annotation.SuppressLint;
import android.app.Activity;
import android.app.Dialog;
import android.content.Intent;
import android.net.Uri;
import android.os.Bundle;
import android.util.DisplayMetrics;
import android.view.LayoutInflater;
import android.view.View;
import android.widget.FrameLayout;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.appcompat.app.AppCompatActivity;
import androidx.fragment.app.FragmentManager;
import androidx.fragment.app.FragmentTransaction;

import com.google.android.material.bottomsheet.BottomSheetBehavior;
import com.google.android.material.bottomsheet.BottomSheetDialog;
import com.google.android.material.bottomsheet.BottomSheetDialogFragment;

import org.chromium.base.Log;
import org.chromium.chrome.R;

public class RewardsTippingSuccessContributionFragment extends BottomSheetDialogFragment {
    final public static String TAG_FRAGMENT = "tipping_success_contribution";
    private static final String AMOUNT_SELECTED = "amount_selected";
    private static final String TAG = "TippingSuccess";

    public static RewardsTippingSuccessContributionFragment newInstance(double amountSelected) {
        RewardsTippingSuccessContributionFragment fragment =
                new RewardsTippingSuccessContributionFragment();
        Bundle args = new Bundle();
        args.putDouble(AMOUNT_SELECTED, amountSelected);
        fragment.setArguments(args);
        return fragment;
    }
    @Override
    public void onCreate(@Nullable Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setStyle(STYLE_NORMAL, R.style.AppBottomSheetDialogTheme);
    }
    @SuppressLint("RestrictedApi")
    @Override
    public void setupDialog(@NonNull Dialog dialog, int style) {
        super.setupDialog(dialog, style);
        final View view =
                LayoutInflater.from(getContext())
                        .inflate(R.layout.rewards_tipping_success_contribution_fragment, null);
        dialog.setContentView(view);
        setShareYourSupportClickListener(view);
        setupFullHeight((BottomSheetDialog) dialog);
        setGoBackClick(view);
    }

    private void setGoBackClick(View view) {
        View goBackButton = view.findViewById(R.id.go_back_button);
        goBackButton.setOnClickListener(v -> { dismiss(); });
    }

    private void setupFullHeight(BottomSheetDialog bottomSheetDialog) {
        FrameLayout bottomSheet =
                (FrameLayout) bottomSheetDialog.findViewById(R.id.design_bottom_sheet);
        BottomSheetBehavior behavior = BottomSheetBehavior.from(bottomSheet);
        behavior.setState(BottomSheetBehavior.STATE_EXPANDED);
    }

    private int getWindowHeight() {
        // Calculate window height for fullscreen use
        DisplayMetrics displayMetrics = new DisplayMetrics();
        ((Activity) getContext()).getWindowManager().getDefaultDisplay().getMetrics(displayMetrics);
        return displayMetrics.heightPixels;
    }

    private void setShareYourSupportClickListener(View view) {
        View shareYourSupportButton = view.findViewById(R.id.share_your_support);
        shareYourSupportButton.setOnClickListener(v -> {
            double amount = 0.0;

            if (getArguments() != null) {
                amount = getArguments().getDouble(AMOUNT_SELECTED);
            }

            String tweetUrl = getTipSuccessTweetUrl(amount);
            Uri uri = Uri.parse(tweetUrl);
            startActivity(new Intent(Intent.ACTION_VIEW, uri));
        });
    }

    @Override
    public void show(@NonNull FragmentManager manager, @Nullable String tag) {
        try {
            RewardsTippingSuccessContributionFragment fragment =
                    (RewardsTippingSuccessContributionFragment) manager.findFragmentByTag(
                            RewardsTippingSuccessContributionFragment.TAG_FRAGMENT);
            FragmentTransaction transaction = manager.beginTransaction();
            if (fragment != null) {
                transaction.remove(fragment);
            }
            transaction.add(this, tag);
            transaction.commitAllowingStateLoss();
        } catch (IllegalStateException e) {
            Log.e(TAG, e.getMessage());
        }
    }

    public static void showTippingSuccessContributionUi(
            AppCompatActivity activity, double amountSelected) {
        if (activity != null) {
            RewardsTippingSuccessContributionFragment braveAskPlayStoreRatingDialog =
                    RewardsTippingSuccessContributionFragment.newInstance(amountSelected);
            braveAskPlayStoreRatingDialog.show(activity.getSupportFragmentManager(), TAG_FRAGMENT);
        }
    }

    private String getTipSuccessTweetUrl(double amount) {
        Uri.Builder builder = new Uri.Builder();
        builder.scheme("https")
                .authority("twitter.com")
                .appendPath("intent")
                .appendPath("tweet")
                .appendQueryParameter("text",
                        String.format(getString(R.string.brave_rewards_tip_success_tweet), amount));
        return builder.build().toString();
    }
}
