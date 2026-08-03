#pragma once

#include <windows.h>
#include <cstdint>

namespace doctor {

class FpsLimiter
{
public:
  void init(int target_fps) noexcept
  {
    _target_fps        = target_fps;
    _target_frame_time = (1000'000 / target_fps) + 1;
    QueryPerformanceFrequency(reinterpret_cast<LARGE_INTEGER*>(&_frequency));
  }

  void update() noexcept
  {
    if (_target_fps > 0)
      fps_limit();
  }

  void start() noexcept
  {
    QueryPerformanceCounter(reinterpret_cast<LARGE_INTEGER*>(&_frame_beg));
  }

private:
  auto elapsed_time_micro() const noexcept
  {
    auto cnt = _frame_end - _frame_beg;
    cnt *= 1000'000;
    cnt /= _frequency;
    return cnt;
  }

  void fps_limit() noexcept
  {
    QueryPerformanceCounter((LARGE_INTEGER*)&_frame_end);
    auto elapsed_time = elapsed_time_micro();
    while (elapsed_time < _target_frame_time)
    {
      if ((elapsed_time + _over_sleep_dur) >= _target_frame_time)
      {
        _over_sleep_dur -= _target_frame_time - elapsed_time;
        break;
      }

      Sleep(1);

      QueryPerformanceCounter((LARGE_INTEGER*)&_frame_end);
      elapsed_time = elapsed_time_micro();

      if (elapsed_time > _target_frame_time)
        _over_sleep_dur += elapsed_time - _target_frame_time;
    }

    QueryPerformanceCounter((LARGE_INTEGER*)&_frame_end);
    _ticks_accumulator += _frame_end - _frame_beg;
    ++_frame_cnt;

    if ((_frame_cnt % _target_fps) == 0)
    {
      _average_fps = ((_frequency * _target_fps) + (_ticks_accumulator - 1)) / _ticks_accumulator;
      _ticks_accumulator = 0;
      _frame_cnt         = 0;
    }

    _frame_beg = _frame_end;
  }

private:
  int      _target_fps{};
  uint32_t _target_frame_time{};
  uint64_t _frequency{};
  int32_t  _frame_cnt{};
  int64_t  _frame_beg{};
  int64_t  _frame_end{};
  int64_t  _average_fps{};
  int64_t  _ticks_accumulator{};
  int64_t  _over_sleep_dur{};
};

}
