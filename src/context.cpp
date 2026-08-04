#include "context.hpp"
#include "WMI.hpp"
#include "common.hpp"

namespace doctor {

void Context::init() noexcept
{
  cpu_infos = g_wmi.get_cpu_infos();
  cpu_infos_get_complete.count_down();
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
