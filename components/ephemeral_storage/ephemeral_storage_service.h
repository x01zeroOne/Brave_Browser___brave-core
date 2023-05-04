/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_EPHEMERAL_STORAGE_EPHEMERAL_STORAGE_SERVICE_H_
#define BRAVE_COMPONENTS_EPHEMERAL_STORAGE_EPHEMERAL_STORAGE_SERVICE_H_

#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/containers/flat_set.h"
#include "base/functional/callback.h"
#include "base/memory/weak_ptr.h"
#include "base/time/time.h"
#include "base/timer/timer.h"
#include "components/content_settings/core/browser/cookie_settings.h"
#include "components/content_settings/core/common/content_settings.h"
#include "components/keyed_service/core/keyed_service.h"
#include "content/public/browser/storage_partition_config.h"
#include "url/gurl.h"
#include "url/origin.h"

class EphemeralStorageBrowserTest;
class EphemeralStorageTest;
class HostContentSettingsMap;

namespace content {
class BrowserContext;
}  // namespace content

namespace permissions {
class PermissionLifetimeManagerBrowserTest;
}

namespace ephemeral_storage {

// Service to enable or disable first party ephemeral storage from external
// actors.
class EphemeralStorageService : public KeyedService {
 public:
  using TLDEphemeralAreaKey =
      std::pair<std::string, content::StoragePartitionConfig>;
  using TLDEphemeralAreaOnDestroyCallbacks =
      std::vector<base::OnceCallback<void(const std::string&)>>;

  EphemeralStorageService(
      content::BrowserContext* context,
      HostContentSettingsMap* host_content_settings_map,
      scoped_refptr<content_settings::CookieSettings> cookie_settings);
  ~EphemeralStorageService() override;

  void Shutdown() override;

  base::WeakPtr<EphemeralStorageService> GetWeakPtr();

  // Performs storage check (cookies, localStorage) and callbacks `true` if
  // nothing is stored in all of these storages.
  void CanEnable1PESForUrl(
      const GURL& url,
      base::OnceCallback<void(bool can_enable_1pes)> callback) const;
  // Enables/disables first party ephemeral storage for |url|.
  void Set1PESEnabledForUrl(const GURL& url, bool enable);
  // Returns current state of first party ephemeral storage mode for |url|.
  bool Is1PESEnabledForUrl(const GURL& url) const;
  // Enables 1PES for url if nothing is stored for |url|.
  void Enable1PESForUrlIfPossible(const GURL& url,
                                  base::OnceCallback<void(bool)> on_ready);

  void TLDEphemeralLifetimeCreated(
      const std::string& ephemeral_domain,
      const content::StoragePartitionConfig& storage_partition_config);
  void TLDEphemeralLifetimeDestroyed(
      const std::string& ephemeral_domain,
      const content::StoragePartitionConfig& storage_partition_config,
      TLDEphemeralAreaOnDestroyCallbacks on_destroy_callbacks);

 private:
  friend EphemeralStorageBrowserTest;
  friend EphemeralStorageTest;
  friend permissions::PermissionLifetimeManagerBrowserTest;

  void FirstPartyStorageAreaInUse(const std::string& ephemeral_domain);
  bool FirstPartyStorageAreaNotInUse(const std::string& ephemeral_domain);

  void OnCanEnable1PESForUrl(const GURL& url,
                             base::OnceCallback<void(bool)> on_ready,
                             bool can_enable_1pes);
  bool IsDefaultCookieSetting(const GURL& url) const;

  void CleanupTLDEphemeralAreaByTimer(
      const TLDEphemeralAreaKey& key,
      bool cleanup_first_party_storage_area,
      TLDEphemeralAreaOnDestroyCallbacks on_destroy_callbacks);
  void CleanupTLDEphemeralArea(
      const TLDEphemeralAreaKey& key,
      bool cleanup_first_party_storage_area,
      TLDEphemeralAreaOnDestroyCallbacks on_destroy_callbacks);

  // If a website was closed, but not yet cleaned-up because of storage lifetime
  // keepalive, we store the origin into a pref to perform a cleanup on browser
  // startup. It's impossible to do a cleanup on shutdown, because the process
  // is asynchronous and cannot block the browser shutdown.
  void ScheduleFirstPartyStorageAreasCleanupOnStartup();
  void CleanupFirstPartyStorageAreasOnStartup();
  void CleanupFirstPartyStorageAreaByTimer(const url::Origin& origin);
  void CleanupFirstPartyStorageArea(const url::Origin& origin);

  size_t FireTLDEphemeralAreaCleanupTimersForTesting();
  size_t FireCleanupTimersForTesting();

  raw_ptr<content::BrowserContext> context_ = nullptr;
  raw_ptr<HostContentSettingsMap> host_content_settings_map_ = nullptr;
  scoped_refptr<content_settings::CookieSettings> cookie_settings_;
  // These patterns are removed on service Shutdown.
  base::flat_set<ContentSettingsPattern> patterns_to_cleanup_on_shutdown_;

  base::TimeDelta tld_ephemeral_area_keep_alive_;
  base::TimeDelta first_party_storage_startup_cleanup_delay_;
  std::map<TLDEphemeralAreaKey, std::unique_ptr<base::OneShotTimer>>
      tld_ephemeral_areas_to_cleanup_;
  base::Value::List first_party_storage_areas_to_cleanup_on_startup_;
  base::OneShotTimer first_party_storage_areas_startup_cleanup_timer_;

  base::WeakPtrFactory<EphemeralStorageService> weak_ptr_factory_{this};
};

}  // namespace ephemeral_storage

#endif  // BRAVE_COMPONENTS_EPHEMERAL_STORAGE_EPHEMERAL_STORAGE_SERVICE_H_
