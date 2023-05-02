/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/ephemeral_storage/ephemeral_storage_service.h"

#include <memory>
#include <utility>
#include <vector>

#include "base/task/sequenced_task_runner.h"
#include "base/time/time.h"
#include "base/timer/timer.h"
#include "brave/components/ephemeral_storage/ephemeral_storage_pref_names.h"
#include "brave/components/ephemeral_storage/url_storage_checker.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "components/prefs/scoped_user_pref_update.h"
#include "components/user_prefs/user_prefs.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/browsing_data_filter_builder.h"
#include "content/public/browser/browsing_data_remover.h"
#include "content/public/browser/dom_storage_context.h"
#include "content/public/browser/site_instance.h"
#include "content/public/browser/storage_partition_config.h"
#include "net/base/features.h"
#include "net/base/schemeful_site.h"
#include "url/origin.h"
#include "url/url_constants.h"

namespace ephemeral_storage {

namespace {

bool IsOriginAcceptableForFirstPartyStorageCleanup(const url::Origin& origin) {
  return !origin.opaque() && (origin.scheme() == url::kHttpScheme ||
                              origin.scheme() == url::kHttpsScheme);
}

url::Origin GetFirstPartyStorageOrigin(const std::string& ephemeral_domain) {
  return url::Origin::CreateFromNormalizedTuple(url::kHttpsScheme,
                                                ephemeral_domain, 443);
}

}  // namespace

EphemeralStorageService::EphemeralStorageService(
    content::BrowserContext* context,
    HostContentSettingsMap* host_content_settings_map,
    scoped_refptr<content_settings::CookieSettings> cookie_settings)
    : context_(context),
      host_content_settings_map_(host_content_settings_map),
      cookie_settings_(std::move(cookie_settings)) {
  DCHECK(context_);
  DCHECK(host_content_settings_map_);
  DCHECK(cookie_settings_);

  tld_ephemeral_area_keep_alive_ = base::Seconds(
      net::features::kBraveEphemeralStorageKeepAliveTimeInSeconds.Get());

  if (base::FeatureList::IsEnabled(
          net::features::kBraveForgetFirstPartyStorage)) {
    ScheduleFirstPartyStorageAreasCleanupOnStartup();
  }
}

EphemeralStorageService::~EphemeralStorageService() = default;

void EphemeralStorageService::Shutdown() {
  for (const auto& pattern : patterns_to_cleanup_on_shutdown_) {
    host_content_settings_map_->SetContentSettingCustomScope(
        pattern, ContentSettingsPattern::Wildcard(),
        ContentSettingsType::COOKIES, CONTENT_SETTING_DEFAULT);
  }
  weak_ptr_factory_.InvalidateWeakPtrs();
}

base::WeakPtr<EphemeralStorageService> EphemeralStorageService::GetWeakPtr() {
  return weak_ptr_factory_.GetWeakPtr();
}

void EphemeralStorageService::CanEnable1PESForUrl(
    const GURL& url,
    base::OnceCallback<void(bool can_enable_1pes)> callback) const {
  if (!IsDefaultCookieSetting(url)) {
    std::move(callback).Run(false);
    return;
  }

  auto site_instance = content::SiteInstance::CreateForURL(context_, url);
  auto* storage_partition = context_->GetStoragePartition(site_instance.get());
  DCHECK(storage_partition);
  base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
      FROM_HERE,
      base::BindOnce(&UrlStorageChecker::StartCheck,
                     base::MakeRefCounted<UrlStorageChecker>(
                         *storage_partition, url, std::move(callback))));
}

void EphemeralStorageService::Set1PESEnabledForUrl(const GURL& url,
                                                   bool enable) {
  auto pattern = ContentSettingsPattern::FromURLNoWildcard(url);
  if (enable) {
    patterns_to_cleanup_on_shutdown_.insert(pattern);
  } else {
    patterns_to_cleanup_on_shutdown_.erase(pattern);
  }
  host_content_settings_map_->SetContentSettingCustomScope(
      pattern, ContentSettingsPattern::Wildcard(), ContentSettingsType::COOKIES,
      enable ? CONTENT_SETTING_SESSION_ONLY : CONTENT_SETTING_DEFAULT);
}

bool EphemeralStorageService::Is1PESEnabledForUrl(const GURL& url) const {
  return host_content_settings_map_->GetContentSetting(
             url, url, ContentSettingsType::COOKIES) ==
         CONTENT_SETTING_SESSION_ONLY;
}

void EphemeralStorageService::Enable1PESForUrlIfPossible(
    const GURL& url,
    base::OnceCallback<void(bool)> on_ready) {
  CanEnable1PESForUrl(
      url,
      base::BindOnce(&EphemeralStorageService::OnCanEnable1PESForUrl,
                     weak_ptr_factory_.GetWeakPtr(), url, std::move(on_ready)));
}

void EphemeralStorageService::OnCanEnable1PESForUrl(
    const GURL& url,
    base::OnceCallback<void(bool)> on_ready,
    bool can_enable_1pes) {
  if (can_enable_1pes) {
    Set1PESEnabledForUrl(url, true);
  }
  std::move(on_ready).Run(can_enable_1pes);
}

bool EphemeralStorageService::IsDefaultCookieSetting(const GURL& url) const {
  ContentSettingsForOneType settings;
  host_content_settings_map_->GetSettingsForOneType(
      ContentSettingsType::COOKIES, &settings);

  for (const auto& setting : settings) {
    if (setting.primary_pattern.Matches(url) &&
        setting.secondary_pattern.Matches(url)) {
      return setting.source == "default";
    }
  }

  return true;
}

void EphemeralStorageService::TLDEphemeralLifetimeCreated(
    const std::string& ephemeral_domain,
    const content::StoragePartitionConfig& storage_partition_config) {
  DVLOG(1) << __func__ << " " << ephemeral_domain << " "
           << storage_partition_config;
  const TLDEphemeralAreaKey key(ephemeral_domain, storage_partition_config);

  tld_ephemeral_areas_to_cleanup_.erase(key);

  FirstPartyStorageAreaInUse(ephemeral_domain);
}

void EphemeralStorageService::TLDEphemeralLifetimeDestroyed(
    const std::string& ephemeral_domain,
    const content::StoragePartitionConfig& storage_partition_config,
    TLDEphemeralAreaOnDestroyCallbacks on_destroy_callbacks) {
  DVLOG(1) << __func__ << " " << ephemeral_domain << " "
           << storage_partition_config;
  const TLDEphemeralAreaKey key(ephemeral_domain, storage_partition_config);

  const bool cleanup_first_party_storage_area =
      FirstPartyStorageAreaNotInUse(ephemeral_domain);

  if (base::FeatureList::IsEnabled(
          net::features::kBraveEphemeralStorageKeepAlive)) {
    auto cleanup_timer = std::make_unique<base::OneShotTimer>();
    cleanup_timer->Start(
        FROM_HERE, tld_ephemeral_area_keep_alive_,
        base::BindOnce(&EphemeralStorageService::CleanupTLDEphemeralAreaByTimer,
                       weak_ptr_factory_.GetWeakPtr(), key,
                       cleanup_first_party_storage_area,
                       std::move(on_destroy_callbacks)));
    tld_ephemeral_areas_to_cleanup_.emplace(key, std::move(cleanup_timer));
  } else {
    CleanupTLDEphemeralArea(key, cleanup_first_party_storage_area,
                            std::move(on_destroy_callbacks));
  }
}

void EphemeralStorageService::FirstPartyStorageAreaInUse(
    const std::string& ephemeral_domain) {
  if (!base::FeatureList::IsEnabled(
          net::features::kBraveForgetFirstPartyStorage)) {
    return;
  }

  const url::Origin origin(GetFirstPartyStorageOrigin(ephemeral_domain));
  if (!IsOriginAcceptableForFirstPartyStorageCleanup(origin)) {
    return;
  }

  const base::Value origin_value(origin.Serialize());
  ScopedListPrefUpdate pref_update(user_prefs::UserPrefs::Get(context_),
                                   kFirstPartyStorageOriginsToCleanup);
  pref_update->EraseValue(origin_value);

  first_party_storage_areas_to_cleanup_on_startup_.EraseValue(origin_value);
}

bool EphemeralStorageService::FirstPartyStorageAreaNotInUse(
    const std::string& ephemeral_domain) {
  if (!base::FeatureList::IsEnabled(
          net::features::kBraveForgetFirstPartyStorage)) {
    return false;
  }

  const url::Origin origin(GetFirstPartyStorageOrigin(ephemeral_domain));
  if (!IsOriginAcceptableForFirstPartyStorageCleanup(origin)) {
    return false;
  }

  const auto& url = origin.GetURL();

  if (host_content_settings_map_->GetContentSetting(
          url, url, ContentSettingsType::BRAVE_SHIELDS) !=
      CONTENT_SETTING_ALLOW) {
    // If Shields are off, do nothing.
    return false;
  }

  if (host_content_settings_map_->GetContentSetting(
          url, url, ContentSettingsType::BRAVE_REMEMBER_1P_STORAGE) !=
      CONTENT_SETTING_BLOCK) {
    return false;
  }

  ScopedListPrefUpdate pref_update(user_prefs::UserPrefs::Get(context_),
                                   kFirstPartyStorageOriginsToCleanup);
  pref_update->Append(base::Value(origin.Serialize()));
  return true;
}

void EphemeralStorageService::CleanupTLDEphemeralAreaByTimer(
    const TLDEphemeralAreaKey& key,
    bool cleanup_first_party_storage_area,
    TLDEphemeralAreaOnDestroyCallbacks on_destroy_callbacks) {
  DVLOG(1) << __func__ << " " << key.first << " " << key.second;
  tld_ephemeral_areas_to_cleanup_.erase(key);
  CleanupTLDEphemeralArea(key, cleanup_first_party_storage_area,
                          std::move(on_destroy_callbacks));
}

void EphemeralStorageService::CleanupTLDEphemeralArea(
    const TLDEphemeralAreaKey& key,
    bool cleanup_first_party_storage_area,
    TLDEphemeralAreaOnDestroyCallbacks on_destroy_callbacks) {
  DVLOG(1) << __func__ << " " << key.first << " " << key.second;
  if (auto* storage_partition = context_->GetStoragePartition(key.second)) {
    auto filter = network::mojom::CookieDeletionFilter::New();
    filter->ephemeral_storage_domain = key.first;
    storage_partition->GetCookieManagerForBrowserProcess()->DeleteCookies(
        std::move(filter), base::NullCallback());
    for (const auto& opaque_origin :
         cookie_settings_->TakeEphemeralStorageOpaqueOrigins(key.first)) {
      storage_partition->GetDOMStorageContext()->DeleteLocalStorage(
          blink::StorageKey::CreateFirstParty(opaque_origin),
          base::DoNothing());
    }
  }
  if (cleanup_first_party_storage_area) {
    CleanupFirstPartyStorageAreaByTimer(GetFirstPartyStorageOrigin(key.first));
  }
  for (auto& callback : on_destroy_callbacks) {
    std::move(callback).Run(key.first);
  }
}

void EphemeralStorageService::CleanupFirstPartyStorageAreaByTimer(
    const url::Origin& origin) {
  CleanupFirstPartyStorageArea(origin);
  ScopedListPrefUpdate pref_update(user_prefs::UserPrefs::Get(context_),
                                   kFirstPartyStorageOriginsToCleanup);
  pref_update->EraseValue(base::Value(origin.Serialize()));
}

void EphemeralStorageService::CleanupFirstPartyStorageArea(
    const url::Origin& origin) {
  DCHECK(base::FeatureList::IsEnabled(
      net::features::kBraveForgetFirstPartyStorage));
  content::BrowsingDataRemover* remover = context_->GetBrowsingDataRemover();
  content::BrowsingDataRemover::DataType data_to_remove =
      content::BrowsingDataRemover::DATA_TYPE_COOKIES |
      content::BrowsingDataRemover::DATA_TYPE_DOM_STORAGE;
  content::BrowsingDataRemover::OriginType origin_type =
      content::BrowsingDataRemover::ORIGIN_TYPE_UNPROTECTED_WEB |
      content::BrowsingDataRemover::ORIGIN_TYPE_PROTECTED_WEB;
  auto filter_builder = content::BrowsingDataFilterBuilder::Create(
      content::BrowsingDataFilterBuilder::Mode::kDelete);
  filter_builder->AddRegisterableDomain(origin.host());
  remover->RemoveWithFilter(base::Time(), base::Time::Max(), data_to_remove,
                            origin_type, std::move(filter_builder));
}

void EphemeralStorageService::ScheduleFirstPartyStorageAreasCleanupOnStartup() {
  first_party_storage_areas_to_cleanup_on_startup_ =
      user_prefs::UserPrefs::Get(context_)
          ->GetList(kFirstPartyStorageOriginsToCleanup)
          .Clone();

  first_party_storage_areas_startup_cleanup_timer_.Start(
      FROM_HERE,
      base::Seconds(
          net::features::
              kBraveForgetFirstPartyStorageStartupCleanupDelayInSeconds.Get()),
      base::BindOnce(
          &EphemeralStorageService::CleanupFirstPartyStorageAreasOnStartup,
          weak_ptr_factory_.GetWeakPtr()));
}

void EphemeralStorageService::CleanupFirstPartyStorageAreasOnStartup() {
  ScopedListPrefUpdate pref_update(user_prefs::UserPrefs::Get(context_),
                                   kFirstPartyStorageOriginsToCleanup);
  for (const auto& url_to_cleanup :
       first_party_storage_areas_to_cleanup_on_startup_) {
    const auto* url_string = url_to_cleanup.GetIfString();
    if (!url_string) {
      continue;
    }
    const GURL url(*url_string);
    if (!url.is_valid()) {
      continue;
    }
    const url::Origin origin(url::Origin::Create(url));
    CleanupFirstPartyStorageArea(origin);

    pref_update->EraseValue(url_to_cleanup);
  }
  first_party_storage_areas_to_cleanup_on_startup_.clear();
}

size_t EphemeralStorageService::FireCleanupTimersForTesting() {
  std::vector<base::OneShotTimer*> timers;
  for (const auto& areas_to_cleanup : tld_ephemeral_areas_to_cleanup_) {
    timers.push_back(areas_to_cleanup.second.get());
  }
  for (auto* timer : timers) {
    timer->FireNow();
  }
  const size_t first_party_storage_areas_to_cleanup_count =
      first_party_storage_areas_to_cleanup_on_startup_.size();
  if (first_party_storage_areas_startup_cleanup_timer_.IsRunning()) {
    first_party_storage_areas_startup_cleanup_timer_.FireNow();
  }
  DCHECK(first_party_storage_areas_to_cleanup_on_startup_.empty());
  return timers.size() + first_party_storage_areas_to_cleanup_count;
}

}  // namespace ephemeral_storage
