#pragma once

#include <windows.h>
#include <print>

namespace doctor {

inline void check(BOOL b, std::string_view msg) noexcept
{
  if (!b)
  {
    std::println("{} Error code = 0x{:08x}", msg, GetLastError());
    exit(EXIT_FAILURE);
  }
}

template <typename... T>
inline void check(BOOL b, std::format_string<T...> const fmt, T&&... args) noexcept
{
  if (!b)
  {
    std::println("{} Error code = 0x{:08x}", std::format(fmt, std::forward<T>(args)...), GetLastError());
    exit(EXIT_FAILURE);
  }
}

inline void check(HRESULT hr, std::string_view msg) noexcept
{
  if (FAILED(hr))
  {
    std::println("{} Error code = 0x{:08X}", msg, static_cast<unsigned long>(hr));
    exit(EXIT_FAILURE);
  }
}

template <typename... T>
inline void check(HRESULT hr, std::format_string<T...> const fmt, T&&... args) noexcept
{
  if (FAILED(hr))
  {
    std::println("{} Error code = 0x{:08X}", std::format(fmt, std::forward<T>(args)...), static_cast<unsigned long>(hr));
    exit(EXIT_FAILURE);
  }
}

}
