// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_SERVICES_DEVICE_GEOLOCATION_GEOCLUE_LOCATION_PROVIDER_H_
#define BRAVE_SERVICES_DEVICE_GEOLOCATION_GEOCLUE_LOCATION_PROVIDER_H_

#include <gio/gio.h>
#include <cstddef>
#include <string>

#include "base/allocator/partition_allocator/pointers/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "base/sequence_checker.h"
#include "base/thread_annotations.h"
#include "services/device/public/cpp/geolocation/geolocation_manager.h"
#include "services/device/public/cpp/geolocation/location_provider.h"
#include "services/device/public/mojom/geoposition.mojom.h"

namespace device {

class GeoClueProvider : public LocationProvider {
 public:
  GeoClueProvider();

  GeoClueProvider(const GeoClueProvider&) = delete;
  GeoClueProvider& operator=(const GeoClueProvider&) = delete;

  ~GeoClueProvider() override;

  // LocationProvider:
  void SetUpdateCallback(
      const LocationProviderUpdateCallback& callback) override;
  void StartProvider(bool high_accuracy) override;
  void StopProvider() override;
  const mojom::Geoposition& GetPosition() override;
  void OnPermissionGranted() override;

 private:
  void SubscribeSignalHandlers();
  void UnsubscribeSignalHandlers();

  static void OnLocationChangedSignal(GDBusConnection* connection,
                                      const char* sender_name,
                                      const char* object_path,
                                      const char* interface_name,
                                      const char* signal_name,
                                      GVariant* parameters,
                                      gpointer user_data);

  SEQUENCE_CHECKER(sequence_checker_);

  raw_ptr<GDBusConnection> connection_ GUARDED_BY_CONTEXT(sequence_checker_) =
      nullptr;
  raw_ptr<GDBusProxy> proxy_ GUARDED_BY_CONTEXT(sequence_checker_) = nullptr;
  raw_ptr<GCancellable> cancellable_ GUARDED_BY_CONTEXT(sequence_checker_) =
      nullptr;

  std::string session_handle_ GUARDED_BY_CONTEXT(sequence_checker_);
  
  guint location_changed_signal_id_ = 0;

  mojom::Geoposition last_position_;
  LocationProviderUpdateCallback location_update_callback_;

  base::WeakPtrFactory<GeoClueProvider> weak_ptr_factory_{this};
};

}  // namespace device

#endif  // BRAVE_SERVICES_DEVICE_GEOLOCATION_GEOCLUE_LOCATION_PROVIDER_H_
