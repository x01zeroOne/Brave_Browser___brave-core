
#include <string>
#include <windows.h>

namespace brave_vpn {

bool ConfigureServiceAutoRestart(const std::wstring& service_name,
                                 const std::wstring& brave_vpn_entry);

bool SetServiceFailActions(SC_HANDLE service);

}  // namespace brave_vpn
