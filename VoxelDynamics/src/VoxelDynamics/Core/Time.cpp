#include "VoxelDynamics/Core/Time.hpp"

#include "SDL3/SDL_timer.h"

namespace VoxelDynamics
{

uint64_t Time::_performance_frequency = 0;
uint64_t Time::_start_counter         = 0;

void Time::Init()
{
    _performance_frequency = SDL_GetPerformanceFrequency();
    _start_counter         = SDL_GetPerformanceCounter();
}

double Time::Seconds()
{
    uint64_t elapsed_ticks = SDL_GetPerformanceCounter() - _start_counter;
    return static_cast<double>(elapsed_ticks) / static_cast<double>(_performance_frequency);
}

} // namespace VoxelDynamics
