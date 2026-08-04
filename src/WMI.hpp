#pragma once

#include "doctor.hpp"

#define _WIN32_DCOM
#include <wbemidl.h>

#include <vector>

namespace doctor {

class WMI
{
public:
  WMI()                      = default;
  ~WMI()                     = default;
  WMI(WMI const&)            = delete;
  WMI(WMI&&)                 = delete;
  WMI& operator=(WMI const&) = delete;
  WMI& operator=(WMI&&)      = delete;

  static auto instance() noexcept -> WMI&
  {
    static WMI wmi;
    return wmi;
  }

  void init() noexcept;
  void destroy() noexcept;

  auto get_cpu_infos() noexcept -> std::vector<CPUInfo>;

private:

  class Enumerator
  {
    friend class WMI;
  public:
    Enumerator()                             = default;
    Enumerator(Enumerator const&)            = delete;
    Enumerator(Enumerator&&)                 = default;
    Enumerator& operator=(Enumerator const&) = delete;
    Enumerator& operator=(Enumerator&&)      = delete;

    ~Enumerator() noexcept
    {
      if (_obj)  _obj->Release();
      if (_enum) _enum->Release();
    }

    auto next() noexcept -> bool;
    auto get_str(std::wstring_view name) const noexcept -> std::string;
    auto get_int(std::wstring_view name) const noexcept -> int;

  private:
    auto get(std::wstring_view name) const noexcept -> VARIANT;

  private:
    IEnumWbemClassObject* _enum{};
    IWbemClassObject*     _obj{};
  };

  auto query(std::wstring_view language, std::wstring_view sql) noexcept -> Enumerator;

private:
  IWbemServices* _service{};
};

inline static auto& g_wmi = WMI::instance();

}
