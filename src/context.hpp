#pragma once

#include <windows.h>

#include <thread>
#include <atomic>
#include <latch>

namespace doctor {

struct Context
{
  static auto instance() noexcept -> Context&
  {
    static Context ctx;
    return ctx;
  }

  std::jthread     thread;
  std::atomic_bool exit{};

  // IWbemRefresher*  refresher{};
  // IWbemHiPerfEnum* enumerate{};

  // std::vector<IWbemObjectAccess*> objects{};

  // long virtual_bytes_handle{};
  // long id_process_handle{};
  // wchar_t const* virtual_bytes_property{};
  // wchar_t const* id_process_property{};

  // CPU usage
  struct CPUTime
  {
    FILETIME idle{};
    FILETIME kernel{};
    FILETIME user{};
  };
  CPUTime    cpu_times[2]{};
  uint32_t   frame_cnt{};
  float      cpu_usage{};

  void get_cpu_information() noexcept;
  std::latch cpu_infos_get_complete{ false };

  void update() noexcept;
  void update_cpu_usage() noexcept;
};

inline static auto& g_ctx = Context::instance();

}
