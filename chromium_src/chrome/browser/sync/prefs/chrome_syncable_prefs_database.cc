/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/sync/prefs/chrome_syncable_prefs_database.h"

#include "base/containers/fixed_flat_map.h"
#include "base/strings/string_piece.h"

namespace browser_sync {
namespace {
namespace brave_syncable_prefs_ids {
enum {
  // Starts at 110000 to avoid clashing with the Chromium's syncable_prefs_ids.
  kProfileContentSettingsExceptionsShieldsAds = 110000,
  kProfileContentSettingsExceptionsTrackers = 110001,
  kProfileContentSettingsExceptionsHttpsUpgrades = 110002,
  kProfileContentSettingsExceptionsHttpUpgradableResources = 110003,
  kProfileContentSettingsExceptionsReferrers = 110004,
  kProfileContentSettingsExceptionsShieldsCookiesV3 = 110005,
  kProfileContentSettingsExceptionsCosmeticFiltering = 110006,
  kProfileContentSettingsExceptionsFingerprintingV2 = 110007,
  kProfileContentSettingsExceptionsBraveShields = 110008,
  kProfileContentSettingsExceptionsBraveSpeedreader = 110009,
  kProfileDefaultContentSettingValuesShieldsAds = 110010,
  kProfileDefaultContentSettingValuesTrackers = 110011,
  kProfileDefaultContentSettingValuesHttpsUpgrades = 110012,
  kProfileDefaultContentSettingValuesHttpUpgradableResources = 110013,
  kProfileDefaultContentSettingValuesReferrers = 110014,
  kProfileDefaultContentSettingValuesShieldsCookiesV3 = 110015,
  kProfileDefaultContentSettingValuesCosmeticFiltering = 110016,
  kProfileDefaultContentSettingValuesFingerprintingV2 = 110017,
  kProfileDefaultContentSettingValuesBraveShields = 110018,
  kProfileDefaultContentSettingValuesBraveSpeedreader = 110019,
};
}  // namespace brave_syncable_prefs_ids

const auto& BraveSyncablePreferences() {
  static const auto kBraveSyncablePrefsAllowList = base::MakeFixedFlatMap<
      base::StringPiece, sync_preferences::SyncablePrefMetadata>({
      {"profile.content_settings.exceptions.shieldsAds",
       {brave_syncable_prefs_ids::kProfileContentSettingsExceptionsShieldsAds,
        syncer::PREFERENCES}},
      {"profile.content_settings.exceptions.trackers",
       {brave_syncable_prefs_ids::kProfileContentSettingsExceptionsTrackers,
        syncer::PREFERENCES}},
      {"profile.content_settings.exceptions.httpsUpgrades",
       {brave_syncable_prefs_ids::
            kProfileContentSettingsExceptionsHttpsUpgrades,
        syncer::PREFERENCES}},
      {"profile.content_settings.exceptions.httpUpgradableResources",
       {brave_syncable_prefs_ids::
            kProfileContentSettingsExceptionsHttpUpgradableResources,
        syncer::PREFERENCES}},
      {"profile.content_settings.exceptions.referrers",
       {brave_syncable_prefs_ids::kProfileContentSettingsExceptionsReferrers,
        syncer::PREFERENCES}},
      {"profile.content_settings.exceptions.shieldsCookiesV3",
       {brave_syncable_prefs_ids::
            kProfileContentSettingsExceptionsShieldsCookiesV3,
        syncer::PREFERENCES}},
      {"profile.content_settings.exceptions.cosmeticFiltering",
       {brave_syncable_prefs_ids::
            kProfileContentSettingsExceptionsCosmeticFiltering,
        syncer::PREFERENCES}},
      {"profile.content_settings.exceptions.fingerprintingV2",
       {brave_syncable_prefs_ids::
            kProfileContentSettingsExceptionsFingerprintingV2,
        syncer::PREFERENCES}},
      {"profile.content_settings.exceptions.braveShields",
       {brave_syncable_prefs_ids::kProfileContentSettingsExceptionsBraveShields,
        syncer::PREFERENCES}},
      {"profile.content_settings.exceptions.braveSpeedreader",
       {brave_syncable_prefs_ids::
            kProfileContentSettingsExceptionsBraveSpeedreader,
        syncer::PREFERENCES}},
      {"profile.default_content_setting_values.shieldsAds",
       {brave_syncable_prefs_ids::kProfileDefaultContentSettingValuesShieldsAds,
        syncer::PREFERENCES}},
      {"profile.default_content_setting_values.trackers",
       {brave_syncable_prefs_ids::kProfileDefaultContentSettingValuesTrackers,
        syncer::PREFERENCES}},
      {"profile.default_content_setting_values.httpsUpgrades",
       {brave_syncable_prefs_ids::
            kProfileDefaultContentSettingValuesHttpsUpgrades,
        syncer::PREFERENCES}},
      {"profile.default_content_setting_values.httpUpgradableResources",
       {brave_syncable_prefs_ids::
            kProfileDefaultContentSettingValuesHttpUpgradableResources,
        syncer::PREFERENCES}},
      {"profile.default_content_setting_values.referrers",
       {brave_syncable_prefs_ids::kProfileDefaultContentSettingValuesReferrers,
        syncer::PREFERENCES}},
      {"profile.default_content_setting_values.shieldsCookiesV3",
       {brave_syncable_prefs_ids::
            kProfileDefaultContentSettingValuesShieldsCookiesV3,
        syncer::PREFERENCES}},
      {"profile.default_content_setting_values.cosmeticFiltering",
       {brave_syncable_prefs_ids::
            kProfileDefaultContentSettingValuesCosmeticFiltering,
        syncer::PREFERENCES}},
      {"profile.default_content_setting_values.fingerprintingV2",
       {brave_syncable_prefs_ids::
            kProfileDefaultContentSettingValuesFingerprintingV2,
        syncer::PREFERENCES}},
      {"profile.default_content_setting_values.braveShields",
       {brave_syncable_prefs_ids::
            kProfileDefaultContentSettingValuesBraveShields,
        syncer::PREFERENCES}},
      {"profile.default_content_setting_values.braveSpeedreader",
       {brave_syncable_prefs_ids::
            kProfileDefaultContentSettingValuesBraveSpeedreader,
        syncer::PREFERENCES}},
  });
  return kBraveSyncablePrefsAllowList;
}
}  // namespace
}  // namespace browser_sync

#define GetSyncablePrefMetadata GetSyncablePrefMetadata_ChromiumImpl
#include "src/chrome/browser/sync/prefs/chrome_syncable_prefs_database.cc"
#undef GetSyncablePrefMetadata

namespace browser_sync {

absl::optional<sync_preferences::SyncablePrefMetadata>
ChromeSyncablePrefsDatabase::GetSyncablePrefMetadata(
    const std::string& pref_name) const {
  const auto* it = BraveSyncablePreferences().find(pref_name);
  if (it != BraveSyncablePreferences().end()) {
    return it->second;
  }
  return GetSyncablePrefMetadata_ChromiumImpl(pref_name);
}

}  // namespace browser_sync
