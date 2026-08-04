#include "ui.hpp"
#include "imgui.h"
#include "doctor.hpp"

#include <array>
#include <chrono>
#include <print>

using namespace doctor;

void ui() noexcept
{
  ImGui::Begin("Doctor");

  // CPU Usage
  {
    static std::array<float, 32> values{};
    static uint32_t              offset{};

    static auto beg_time = std::chrono::high_resolution_clock::now();
    static auto accur_time = 0;
    auto curr_time = std::chrono::high_resolution_clock::now();
    auto dur       = std::chrono::duration_cast<std::chrono::milliseconds>(curr_time - beg_time).count();
    accur_time += dur;
    beg_time   = curr_time;
    if (accur_time > 1'000)
      values[offset % values.size()] = get_cpu_usage();
    ImGui::PlotLines("CPU Usage", values.data(), values.size(), offset, nullptr, 0.f, 100.f, ImVec2(0, 80.f));
    if (accur_time > 1'000)
    {
      offset = (offset + 1) % values.size();
      accur_time = 0;
    }
  }

  ImGui::End();
}
