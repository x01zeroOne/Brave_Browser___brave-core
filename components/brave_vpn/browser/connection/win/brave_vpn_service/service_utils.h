
#include <string>
#include <windows.h>

namespace brave_vpn {

bool ConfigureServiceAutoRestart(const std::wstring& service_name);

bool SetServiceFailActions(SC_HANDLE service);
std::wstring GetVpnServiceName();
}  // namespace brave_vpn
