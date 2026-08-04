#pragma once

namespace doctor {

void run_system_diagnostics_thead() noexcept;
void exit_system_diagnostics_thread() noexcept;

auto get_cpu_usage() noexcept -> float;

}
