/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CREATIVES_NEW_TAB_PAGE_ADS_CREATIVE_NEW_TAB_PAGE_ADS_DATABASE_TABLE_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CREATIVES_NEW_TAB_PAGE_ADS_CREATIVE_NEW_TAB_PAGE_ADS_DATABASE_TABLE_H_

#include <string>
#include <vector>

#include "base/check_op.h"
#include "base/functional/callback.h"
#include "brave/components/brave_ads/core/internal/account/deposits/deposits_database_table.h"
#include "brave/components/brave_ads/core/internal/creatives/campaigns_database_table.h"
#include "brave/components/brave_ads/core/internal/creatives/creative_ads_database_table.h"
#include "brave/components/brave_ads/core/internal/creatives/dayparts_database_table.h"
#include "brave/components/brave_ads/core/internal/creatives/geo_targets_database_table.h"
#include "brave/components/brave_ads/core/internal/creatives/new_tab_page_ads/creative_new_tab_page_ad_info.h"
#include "brave/components/brave_ads/core/internal/creatives/new_tab_page_ads/creative_new_tab_page_ad_wallpapers_database_table.h"
#include "brave/components/brave_ads/core/internal/creatives/segments_database_table.h"
#include "brave/components/brave_ads/core/internal/database/database_table_interface.h"
#include "brave/components/brave_ads/core/internal/segments/segment_alias.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"
#include "brave/components/brave_ads/core/public/client/ads_client_callback.h"

namespace brave_ads::database::table {

using GetCreativeNewTabPageAdCallback =
    base::OnceCallback<void(bool success,
                            const std::string& creative_instance_id,
                            const CreativeNewTabPageAdInfo& creative_ad)>;

using GetCreativeNewTabPageAdsCallback =
    base::OnceCallback<void(bool success,
                            const std::vector<std::string>& segments,
                            const CreativeNewTabPageAdList& creative_ads)>;

class CreativeNewTabPageAds final : public TableInterface {
 public:
  CreativeNewTabPageAds();

  CreativeNewTabPageAds(const CreativeNewTabPageAds&) = delete;
  CreativeNewTabPageAds& operator=(const CreativeNewTabPageAds&) = delete;

  CreativeNewTabPageAds(CreativeNewTabPageAds&&) noexcept = delete;
  CreativeNewTabPageAds& operator=(CreativeNewTabPageAds&&) noexcept = delete;

  ~CreativeNewTabPageAds() override;

  void Save(const CreativeNewTabPageAdList& creative_ads,
            ResultCallback callback);

  void Delete(ResultCallback callback) const;

  void GetForCreativeInstanceId(const std::string& creative_instance_id,
                                GetCreativeNewTabPageAdCallback callback) const;

  void GetForSegments(const SegmentList& segments,
                      GetCreativeNewTabPageAdsCallback callback) const;

  void GetAll(GetCreativeNewTabPageAdsCallback callback) const;

  void SetBatchSize(const int batch_size) {
    CHECK_GT(batch_size, 0);

    batch_size_ = batch_size;
  }

  std::string GetTableName() const override;

  void Create(mojom::DBTransactionInfo* transaction) override;
  void Migrate(mojom::DBTransactionInfo* transaction, int to_version) override;

 private:
  void InsertOrUpdate(mojom::DBTransactionInfo* transaction,
                      const CreativeNewTabPageAdList& creative_ads);

  std::string BuildInsertOrUpdateSql(
      mojom::DBCommandInfo* command,
      const CreativeNewTabPageAdList& creative_ads) const;

  int batch_size_;

  Campaigns campaigns_database_table_;
  CreativeAds creative_ads_database_table_;
  CreativeNewTabPageAdWallpapers
      creative_new_tab_page_ad_wallpapers_database_table_;
  Dayparts dayparts_database_table_;
  Deposits deposits_database_table_;
  GeoTargets geo_targets_database_table_;
  Segments segments_database_table_;
};

}  // namespace brave_ads::database::table

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CREATIVES_NEW_TAB_PAGE_ADS_CREATIVE_NEW_TAB_PAGE_ADS_DATABASE_TABLE_H_
