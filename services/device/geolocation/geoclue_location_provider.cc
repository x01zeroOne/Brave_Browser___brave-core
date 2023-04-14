// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include </usr/include/libgeoclue-2.0/gclue-simple.h>

#include <gio/gcancellable.h>

#include "base/functional/bind.h"
#include "base/sequence_checker.h"
#include "brave/services/device/geolocation/geoclue_location_provider.h"
#include "services/device/public/mojom/geoposition.mojom.h"
namespace device {

namespace {
  constexpr char kGeoClueInterfaceName[] = "org.freedesktop.GeoClue2.Manager";
}

GeoClueProvider::GeoClueProvider() {
  DETACH_FROM_SEQUENCE(sequence_checker_);
}
GeoClueProvider::~GeoClueProvider() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  UnsubscribeSignalHandlers();
}

void GeoClueProvider::SetUpdateCallback(
    const LocationProviderUpdateCallback& callback) {}

void GeoClueProvider::StartProvider(bool high_accuracy) {
  auto accuracy = high_accuracy ? GCLUE_ACCURACY_LEVEL_EXACT
                                : GCLUE_ACCURACY_LEVEL_NEIGHBORHOOD;

  GError* error = nullptr;

  CHECK(!error);

  auto* location = gclue_simple_get_location(gclue_simple_);
  last_position_.accuracy = gclue_location_get_accuracy(location);
  last_position_.altitude = gclue_location_get_altitude(location);
  last_position_.heading = gclue_location_get_heading(location);
  last_position_.latitude = gclue_location_get_latitude(location);
  last_position_.longitude = gclue_location_get_longitude(location);
  last_position_.speed = gclue_location_get_speed(location);

  // TODO: Work out how to get this from a GVariant.
  last_position_.timestamp = base::Time::Now();
}
void GeoClueProvider::StopProvider() {}

const mojom::Geoposition& GeoClueProvider::GetPosition() {
  return last_position_;
}

void GeoClueProvider::OnPermissionGranted() {}

void GeoClueProvider::SubscribeSignalHandlers() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  location_changed_signal_id_ = g_dbus_connection_signal_subscribe(connection_, const gchar *sender, const gchar *interface_name, const gchar *member, const gchar *object_path, const gchar *arg0, GDBusSignalFlags flags, GDBusSignalCallback callback, gpointer user_data, GDestroyNotify user_data_free_func)
}

void GeoClueProvider::UnsubscribeSignalHandlers() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  if (location_changed_signal_id_) {
    g_dbus_connection_signal_unsubscribe(connection_,
                                         location_changed_signal_id_);
    location_changed_signal_id_ = 0;
  }
}

}  // namespace device
