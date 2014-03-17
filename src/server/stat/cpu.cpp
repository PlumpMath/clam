/*
  Copyright © 2014 Jesse 'Jeaye' Wilkerson
  See licensing in LICENSE file, or at:
    http://www.opensource.org/licenses/MIT

  File: cpu.cpp
  Author: Jesse 'Jeaye' Wilkerson
*/

#include "cpu.h"

#include <chrono>
#include <thread>
#include <cstdlib> // TODO: Use better rand

#ifdef __APPLE__
# include <mach/mach_init.h>
# include <mach/mach_error.h>
# include <mach/mach_host.h>
# include <mach/vm_map.h>
#endif

namespace server
{
  namespace stat
  {
    static float pretend_cpu_load()
    {
      static float last{};
      static constexpr const float min{ 0.01371f };
      static constexpr const float max{ 0.51270f };
      static constexpr const float diff_min{ 0.0042f };
      static constexpr const float diff_max{ 0.018f };

      float diff{ ((diff_max - diff_min) *
                  (static_cast<float>(rand()) / RAND_MAX)) + diff_min };
      diff *= (rand() % 2) ? 1.0f : -1.0f;
      last += diff;
      last = std::max(last, min);
      last = std::min(last, max);
      return last;
    }

#ifdef __APPLE__
    static float calculate_cpu(uint64_t const idle, uint64_t const total)
    {
      static uint64_t previous_total{};
      static uint64_t previous_idle{};

      uint64_t const delta_total{ total - previous_total };
      uint64_t const delta_idle{ idle - previous_idle };
      float const ret{ (delta_total > 0) ?
                       (static_cast<float>(delta_idle)) / delta_total : 0 };
      previous_total = total;
      previous_idle = idle;

      return 1.0f - ret;
    }

    static float cpu_load()
    {
      host_cpu_load_info_data_t cpuinfo;
      mach_msg_type_number_t count{ HOST_CPU_LOAD_INFO_COUNT };
      if(host_statistics(mach_host_self(), HOST_CPU_LOAD_INFO,
                         reinterpret_cast<host_info_t>(&cpuinfo), &count) == KERN_SUCCESS)
      {
        uint64_t total{};
        for(int i{}; i < CPU_STATE_MAX; ++i)
        { total += cpuinfo.cpu_ticks[i]; }
        return calculate_cpu(cpuinfo.cpu_ticks[CPU_STATE_IDLE], total);
      }
      else
      { return pretend_cpu_load(); }
    }
#else
    static float cpu_load()
    {
      return pretend_cpu_load();
    }
#endif

    std::string cpu_bar(size_t const width)
    {
      if(width < 2)
      { return ""; }

      float const percent{ cpu_load() * 100.0f };
      std::string bar(width, ' ');
      bar[0] = '['; bar[width - 1] = ']';
      for(size_t i{}; i < width - 2; ++i)
      {
        if(percent > i * (100.0f / (width - 2)))
        { bar[i + 1] = '|'; }
      }
      return bar;
    }
  }
}