// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_BRAVE_NEWS_BROWSER_BRAVE_NEWS_P3A_H_
#define BRAVE_COMPONENTS_BRAVE_NEWS_BROWSER_BRAVE_NEWS_P3A_H_

#include <cstdint>

class PrefRegistrySimple;
class PrefService;

namespace brave_news {
namespace p3a {

constexpr char kDaysInMonthUsedCountHistogramName[] =
    "Brave.Today.DaysInMonthUsedCount";
constexpr char kWeeklySessionCountHistogramName[] =
    "Brave.Today.WeeklySessionCount";
constexpr char kTotalCardViewsHistogramName[] =
    "Brave.Today.WeeklyTotalCardViews";
constexpr char kWeeklyDisplayAdsViewedHistogramName[] =
    "Brave.Today.WeeklyDisplayAdsViewedCount";
constexpr char kDirectFeedsTotalHistogramName[] =
    "Brave.Today.DirectFeedsTotal";
constexpr char kWeeklyAddedDirectFeedsHistogramName[] =
    "Brave.Today.WeeklyAddedDirectFeedsCount";
constexpr char kLastUsageTimeHistogramName[] = "Brave.Today.LastUsageTime";
constexpr char kNewUserReturningHistogramName[] =
    "Brave.Today.NewUserReturning";

void RecordAtInit(PrefService* prefs);
void RecordAtSessionStart(PrefService* prefs);

void RecordWeeklyDisplayAdsViewedCount(PrefService* prefs, bool is_add);
void RecordWeeklyAddedDirectFeedsCount(PrefService* prefs, int change);
void RecordDirectFeedsTotal(PrefService* prefs);
void RecordTotalCardViews(PrefService* prefs, uint64_t count_delta);

void RegisterProfilePrefs(PrefRegistrySimple* registry);
void RegisterProfilePrefsForMigration(PrefRegistrySimple* registry);
void MigrateObsoleteProfilePrefs(PrefService* prefs);

}  // namespace p3a
}  // namespace brave_news

#endif  // BRAVE_COMPONENTS_BRAVE_NEWS_BROWSER_BRAVE_NEWS_P3A_H_
