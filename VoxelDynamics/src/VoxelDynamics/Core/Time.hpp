#pragma once

namespace VoxelDynamics
{

class Time
{
public:
    static void Init();

    static double Seconds();

private:
    static uint64_t _performance_frequency, _start_counter;
};

} // namespace VoxelDynamics
