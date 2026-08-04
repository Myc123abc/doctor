#pragma once

#include <string>
#include <span>

namespace doctor {

struct CPUInfo
{
  std::string name;
  uint32_t    core_num{};
};

void run_system_diagnostics_thead() noexcept;
void exit_system_diagnostics_thread() noexcept;

auto get_cpu_usage() noexcept -> float;
auto get_cpu_infos() noexcept -> std::span<CPUInfo>;

}
