#include "doctor.hpp"
#include "fps_limiter.hpp"

#define _WIN32_DCOM
#include <wbemidl.h>
#pragma comment(lib, "wbemuuid.lib")

#include <thread>
#include <print>
#include <string_view>
#include <atomic>
#include <vector>

namespace {

struct Context
{
  std::jthread     thread;
  std::atomic_bool exit{};

  IWbemRefresher*  refresher{};
  IWbemHiPerfEnum* enumerate{};

  std::vector<IWbemObjectAccess*> objects{};

  long virtual_bytes_handle{};
  long id_process_handle{};
  wchar_t const* virtual_bytes_property{};
  wchar_t const* id_process_property{};

  // CPU usage
  struct CPUTime
  {
    FILETIME idle{};
    FILETIME kernel{};
    FILETIME user{};
  };
  CPUTime   cpu_times[2]{};
  uint32_t  frame_cnt{};
  float     cpu_usage{};

  void update() noexcept;
  void update_cpu_usage() noexcept;
} g_ctx;

void check(BOOL b, std::string_view msg) noexcept
{
  if (!b)
  {
    std::println("{} Error code = 0x{:08x}", msg, GetLastError());
    exit(EXIT_FAILURE);
  }
}

template <typename... T>
void check(BOOL b, std::format_string<T...> const fmt, T&&... args) noexcept
{
  if (!b)
  {
    std::println("{} Error code = 0x{:08x}", std::format(fmt, std::forward<T>(args)...), GetLastError());
    exit(EXIT_FAILURE);
  }
}

void check(HRESULT hr, std::string_view msg) noexcept
{
  if (FAILED(hr))
  {
    std::println("{} Error code = 0x{:08X}", msg, static_cast<unsigned long>(hr));
    exit(EXIT_FAILURE);
  }
}

template <typename... T>
void check(HRESULT hr, std::format_string<T...> const fmt, T&&... args) noexcept
{
  if (FAILED(hr))
  {
    std::println("{} Error code = 0x{:08X}", std::format(fmt, std::forward<T>(args)...), static_cast<unsigned long>(hr));
    exit(EXIT_FAILURE);
  }
}

void init() noexcept
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
  IWbemServices* svc{};
  check(loc->ConnectServer(str, NULL, NULL, 0, NULL, 0, 0, &svc),
    "Could not connect.");
  SysFreeString(str);
  loc->Release();

  // Set the proxy so that impersonation of the client occurs.
  check(CoSetProxyBlanket(svc, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE, NULL, RPC_C_AUTHN_LEVEL_CALL, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE),
    "Could not set proxy blanket.");

  // Create refresher.
  check(CoCreateInstance(CLSID_WbemRefresher, NULL, CLSCTX_INPROC_SERVER, IID_IWbemRefresher, reinterpret_cast<LPVOID*>(&g_ctx.refresher)),
    "Failed to create refresher.");

  // Add an enumerator to the refresher.
  IWbemConfigureRefresher* cfg{};
  check(g_ctx.refresher->QueryInterface(IID_IWbemConfigureRefresher, reinterpret_cast<LPVOID*>(&cfg)),
    "Failed to get configure refresher object.");
  long id{};
  constexpr auto invalid_class_hr = static_cast<HRESULT>(0x80041010L);
  auto hr = cfg->AddEnum(svc, L"Win32_PerfRawData_PerfProc_Process", 0, NULL, &g_ctx.enumerate, &id);
  g_ctx.virtual_bytes_property = L"VirtualBytes";
  g_ctx.id_process_property = L"IDProcess";
  if (FAILED(hr) && hr == invalid_class_hr)
  {
    std::println("The performance counter class is unavailable on this machine. Falling back to Win32_Process.");
    hr = cfg->AddEnum(svc, L"Win32_Process", 0, NULL, &g_ctx.enumerate, &id);
    g_ctx.virtual_bytes_property = L"VirtualSize";
    g_ctx.id_process_property = L"ProcessId";
  }
  check(hr, "Failed to add an enumerator to the refresher.");
  cfg->Release();
  svc->Release();
}

void destroy() noexcept
{
  g_ctx.refresher->Release();
  CoUninitialize();
}

void Context::update() noexcept
{
  update_cpu_usage();
  // Refresh the refresher.
  // check(refresher->Refresh(0L), "Failed to refresh the refresher.");

  // Get the objects.
//   DWORD cnt{};
// label_reget_objects:
//   auto res = g_ctx.enumerate->GetObjects(0L, objects.size(), objects.data(), &cnt);
//   if (res == WBEM_E_BUFFER_TOO_SMALL && cnt > objects.size())
//   {
//     objects.resize(cnt);
//     goto label_reget_objects;
//   }
//   check(res, "Failed to get objects.");

  // First time through, get the handles.
  // static auto first_time{ true };
  // if (first_time)
  // {
  //   first_time = false;
  //   CIMTYPE virtual_bytes_type{};
  //   CIMTYPE process_handle_type{};
  //   auto const& obj = objects[0];
  //   check(obj->GetPropertyHandle(virtual_bytes_property, &virtual_bytes_type, &virtual_bytes_handle),
  //     "Failed to get virtual size handle");
  //   check(obj->GetPropertyHandle(id_process_property, &process_handle_type, &id_process_handle),
  //     "Failed to get ID process handle");
  // }

  // for (auto i = 0; i < cnt; ++i)
  // {
  //   auto const& obj = objects[i];
  //   ULONGLONG virtual_bytes{};
  //   check(obj->ReadQWORD(virtual_bytes_handle, &virtual_bytes), "Failed to get virtual size.");
  //   DWORD id_process{};
  //   check(obj->ReadDWORD(id_process_handle, &id_process), "Failed to get ID process.");
  //   std::println("Process ID {} is using {} bytes", id_process, virtual_bytes);
  //   obj->Release();
  // }
  ++frame_cnt;
}

void Context::update_cpu_usage() noexcept
{
  auto       & curr_cpu_time = cpu_times[frame_cnt       % 2];
  auto const & prev_cpu_time = cpu_times[(frame_cnt + 1) % 2];

  check(GetSystemTimes(&curr_cpu_time.idle, &curr_cpu_time.kernel, &curr_cpu_time.user),
    "Failed to get system times");
  if (frame_cnt)
  {
    auto delta = [](FILETIME const& lhs, FILETIME const& rhs)
    {
      return ((static_cast<ULONGLONG>(lhs.dwHighDateTime) << 32) | lhs.dwLowDateTime) -
             ((static_cast<ULONGLONG>(rhs.dwHighDateTime) << 32) | rhs.dwLowDateTime);
    };
    auto idle   = delta(curr_cpu_time.idle,   prev_cpu_time.idle);
    auto kernel = delta(curr_cpu_time.kernel, prev_cpu_time.kernel);
    auto user   = delta(curr_cpu_time.user,   prev_cpu_time.user);
    auto total  = kernel + user;

    cpu_usage = static_cast<float>(total - idle) / total * 100;
  }
}

}

namespace doctor {

void run_system_diagnostics_thead() noexcept
{
  g_ctx.thread = std::jthread([]
  {
    init();

    auto limiter = FpsLimiter{};
    limiter.init(1);

    limiter.start();
    while (!g_ctx.exit.load(std::memory_order_relaxed))
    {
      g_ctx.update();
      limiter.update();
    }

    destroy();
  });
}

void exit_system_diagnostics_thread() noexcept
{
  g_ctx.exit.store(true, std::memory_order_relaxed);
  g_ctx.thread.join();
}

auto get_cpu_usage() noexcept -> float
{
  return g_ctx.cpu_usage;
}

}
