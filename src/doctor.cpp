#include "doctor.hpp"
#include "WMI.hpp"
#include "context.hpp"
#include "fps_limiter.hpp"

using namespace doctor;

  // // Create refresher.
  // check(CoCreateInstance(CLSID_WbemRefresher, NULL, CLSCTX_INPROC_SERVER, IID_IWbemRefresher, reinterpret_cast<LPVOID*>(&g_ctx.refresher)),
  //   "Failed to create refresher.");

  // // Add an enumerator to the refresher.
  // IWbemConfigureRefresher* cfg{};
  // check(g_ctx.refresher->QueryInterface(IID_IWbemConfigureRefresher, reinterpret_cast<LPVOID*>(&cfg)),
  //   "Failed to get configure refresher object.");
  // long id{};
  // constexpr auto invalid_class_hr = static_cast<HRESULT>(0x80041010L);
  // auto hr = cfg->AddEnum(svc, L"Win32_PerfRawData_PerfProc_Process", 0, NULL, &g_ctx.enumerate, &id);
  // g_ctx.virtual_bytes_property = L"VirtualBytes";
  // g_ctx.id_process_property = L"IDProcess";
  // if (FAILED(hr) && hr == invalid_class_hr)
  // {
  //   std::println("The performance counter class is unavailable on this machine. Falling back to Win32_Process.");
  //   hr = cfg->AddEnum(svc, L"Win32_Process", 0, NULL, &g_ctx.enumerate, &id);
  //   g_ctx.virtual_bytes_property = L"VirtualSize";
  //   g_ctx.id_process_property = L"ProcessId";
  // }
  // check(hr, "Failed to add an enumerator to the refresher.");
  // cfg->Release();

namespace doctor {

void run_system_diagnostics_thead() noexcept
{
  g_ctx.thread = std::jthread([]
  {
    g_wmi.init();
    g_ctx.init();

    auto limiter = FpsLimiter{};
    limiter.init(1);

    limiter.start();
    while (!g_ctx.exit.load(std::memory_order_relaxed))
    {
      g_ctx.update();
      limiter.update();
    }
  });
}

void exit_system_diagnostics_thread() noexcept
{
  g_ctx.exit.store(true, std::memory_order_relaxed);
  g_wmi.destroy();
  g_ctx.thread.join();
}

auto get_cpu_usage() noexcept -> float
{
  return g_ctx.cpu_usage;
}

auto get_cpu_infos() noexcept -> std::span<CPUInfo>
{
  if (g_ctx.cpu_infos_get_complete.try_wait())
    return g_ctx.cpu_infos;
  return {};
}

}
