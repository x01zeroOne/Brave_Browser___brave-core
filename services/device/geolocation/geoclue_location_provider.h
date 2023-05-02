// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_SERVICES_DEVICE_GEOLOCATION_GEOCLUE_LOCATION_PROVIDER_H_
#define BRAVE_SERVICES_DEVICE_GEOLOCATION_GEOCLUE_LOCATION_PROVIDER_H_

#include <memory>
#include <string>

#include "base/memory/scoped_refptr.h"
#include "base/memory/weak_ptr.h"
#include "base/sequence_checker.h"
#include "dbus/bus.h"
#include "dbus/message.h"
#include "dbus/object_path.h"
#include "dbus/object_proxy.h"
#include "dbus/property.h"
#include "services/device/public/cpp/geolocation/location_provider.h"
#include "services/device/public/mojom/geoposition.mojom.h"

namespace device {

class GeolocationManager;

struct GeoClueProperties : public dbus::PropertySet {
  dbus::Property<std::string> desktop_id;

  GeoClueProperties(dbus::ObjectProxy *proxy, const std::string &interface_name,
                    const PropertyChangedCallback &callback);
  ~GeoClueProperties() override;
};

struct GeoClueLocationProperties : public dbus::PropertySet {
  dbus::Property<double> latitude;
  dbus::Property<double> longitude;
  dbus::Property<double> accuracy;
  dbus::Property<double> altitude;
  dbus::Property<double> speed;
  dbus::Property<double> heading;

  GeoClueLocationProperties(dbus::ObjectProxy *proxy,
                            const std::string &interface_name,
                            base::OnceCallback<void()> on_got_initial_values);
  ~GeoClueLocationProperties() override;

  // dbus::PropertySet:
  void OnGetAll(dbus::Response *response) override;

private:
  base::OnceCallback<void()> on_got_initial_values_;
};

class GeoClueLocationProvider : public LocationProvider {
public:
  GeoClueLocationProvider();

  GeoClueLocationProvider(const GeoClueLocationProvider &) = delete;
  GeoClueLocationProvider &operator=(const GeoClueLocationProvider &) = delete;

  ~GeoClueLocationProvider() override;

  // LocationProvider:
  void
  SetUpdateCallback(const LocationProviderUpdateCallback &callback) override;
  void StartProvider(bool high_accuracy) override;
  void StopProvider() override;
  const mojom::Geoposition &GetPosition() override;
  void OnPermissionGranted() override;

protected:
  enum ClientState {
    kStopped,
    kInitializing,
    kInitialized,
    kStarting,
    kStarted,
  };

  ClientState client_state_ = ClientState::kStopped;

  // Stores whether or not permission has been granted.
  bool permission_granted_ = false;

  void SetLocation(const mojom::Geoposition &position);

private:
  // There is a bit of a process to setup a GeoClue2.Client and start listening
  // for location changes:
  // 1. Get the current GeoClue2.Manager
  // 2. Call Manager.GetClient(), which returns a client
  // 3. Set the DesktopId for ourselves. This is basically just an identifier
  // for the current app.
  // 4. Connect to the `LocationUpdated` signal, which will fire when we get a
  // location.
  // 5. Finally, we can call GeoClue2.Client.Start(), which will let us access
  // the current location.
  // In this process, it's safe to do steps 1, 2, 3 & 4 before permission is
  // granted, but step 5 should not be called until permission has been granted.
  //
  // If any step fails, we set the current position to POSITION_UNAVAILABLE.
  //
  // When the provider is stopped this process is completely reset.

  // Step 2
  void GetClient();
  void OnGetClientCompleted(dbus::Response *response);

  // Step 3
  void SetDesktopId();
  void OnSetDesktopId(bool success);

  // Step 4
  void ConnectSignal();
  void OnSignalConnected(const std::string &interface_name,
                         const std::string &signal_name, bool success);

  // Step 5: Start the client. This is safe to call before permission is granted
  // or while the client is starting up, it will just be ignored if we aren't
  // ready to start. It will be invoked again when everything is ready.
  void StartClient();
  void OnClientStarted(dbus::Response *response);

  // Functions for triggering the read of a new GeoClue2.Location, and a
  // callback for when it has been read.
  void ReadGeoClueLocation(const dbus::ObjectPath &path);
  void OnReadGeoClueLocation();

  SEQUENCE_CHECKER(sequence_checker_);

  scoped_refptr<dbus::Bus> bus_;
  scoped_refptr<dbus::ObjectProxy> gclue_client_;

  std::unique_ptr<GeoClueProperties> gclue_client_properties_;
  std::unique_ptr<GeoClueLocationProperties> gclue_location_properties_;

  mojom::Geoposition last_position_;
  LocationProviderUpdateCallback location_update_callback_;

  base::WeakPtrFactory<GeoClueLocationProvider> weak_ptr_factory_{this};
};

} // namespace device

#endif // BRAVE_SERVICES_DEVICE_GEOLOCATION_GEOCLUE_LOCATION_PROVIDER_H_
