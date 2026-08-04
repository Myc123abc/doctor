#include "WMI.hpp"
#include "common.hpp"

#pragma comment(lib, "wbemuuid.lib")

namespace {

auto to_str(wchar_t const* wstr) noexcept -> std::string
{
  if (!wstr) return {};
  auto size = WideCharToMultiByte(CP_UTF8, 0, wstr, -1, nullptr, 0, nullptr, nullptr);
  auto result = std::string(size - 1, '\0');
  WideCharToMultiByte(CP_UTF8, 0, wstr, -1, result.data(), size, nullptr, nullptr);
  return result;
}

auto to_int(VARIANT const& value) noexcept -> int
{
  switch (value.vt)
  {
  case VT_I4:  return static_cast<int>(value.lVal);
  case VT_UI4: return static_cast<int>(value.ulVal);
  case VT_I2:  return static_cast<int>(value.iVal);
  case VT_UI2: return static_cast<int>(value.uiVal);
  default: return 0;
  }
}

}

namespace doctor {

void WMI::init() noexcept
{
  // Initialize COM.
  check(CoInitializeEx(0, COINIT_APARTMENTTHREADED), "Failed to initialize COM library.");

  // Set COM security levels.
  check(CoInitializeSecurity(NULL, -1, NULL, NULL,
    RPC_C_AUTHN_LEVEL_DEFAULT, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE, NULL),
    "Failed to initialize security.");

  // Create a connection to a WMI namespace.

  // Initialize IWbemLocator.
  IWbemLocator* loc{};
  check(CoCreateInstance(CLSID_WbemLocator, 0, CLSCTX_INPROC_SERVER, IID_IWbemLocator, reinterpret_cast<LPVOID*>(&loc)),
    "Failed to create IWbemLocator object.");

  // Connect to WMI.
  auto str = SysAllocString(L"\\\\.\\root\\cimv2");
  check(loc->ConnectServer(str, NULL, NULL, 0, NULL, 0, 0, &_service),
    "Could not connect.");
  SysFreeString(str);
  loc->Release();

  // Set the proxy so that impersonation of the client occurs.
  check(CoSetProxyBlanket(_service, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE, NULL, RPC_C_AUTHN_LEVEL_CALL, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE),
    "Could not set proxy blanket.");
}

void WMI::destroy() noexcept
{
  _service->Release();
  CoUninitialize();
}

auto WMI::query(std::wstring_view language, std::wstring_view sql) noexcept -> Enumerator
{
  auto enumerator  = Enumerator{};
  auto lang        = SysAllocString(language.data());
  auto sql_content = SysAllocString(sql.data());
  check(_service->ExecQuery(lang, sql_content, WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY, nullptr, &enumerator._enum),
    "Failed to execute query.");
  SysFreeString(lang);
  SysFreeString(sql_content);
  return enumerator;
}

auto WMI::Enumerator::next() noexcept -> bool
{
  if (_obj) _obj->Release();

  ULONG res;
  check(_enum->Next(WBEM_INFINITE, 1, &_obj, &res),
    "Failed to get next object in enumerator");
  if (!res) _obj = nullptr;
  return _obj;
}

auto WMI::Enumerator::get(std::wstring_view name) const noexcept -> VARIANT
{
  auto value = VARIANT{};
  check(_obj->Get(name.data(), 0, &value, nullptr, nullptr), "Failed to get property from object");
  return value;
}

auto WMI::Enumerator::get_str(std::wstring_view name) const noexcept -> std::string
{
  return to_str(get(name).bstrVal);
}

auto WMI::Enumerator::get_int(std::wstring_view name) const noexcept -> int
{
  return to_int(get(name));
}

auto WMI::get_cpu_infos() noexcept -> std::vector<CPUInfo>
{
  auto infos      = std::vector<CPUInfo>{};
  auto enumerator = query(L"WQL", L"SELECT * FROM Win32_Processor");
  while (enumerator.next())
  {
    auto& info = infos.emplace_back();
    info.name     = enumerator.get_str(L"Name");
    info.core_num = enumerator.get_int(L"NumberOfCores");
  }
  return infos;
}

}
