#include "VoxelDynamics/Core/Log.hpp"

#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/spdlog.h"

namespace VoxelDynamics
{

std::shared_ptr<spdlog::logger> Log::CoreLogger;
std::shared_ptr<spdlog::logger> Log::ClientLogger;

void Log::SetLevel(Level level)
{
    SetCoreLevel(level);
    SetClientLevel(level);
}

void Log::SetCoreLevel(Level level)
{
    CoreLogger->set_level(static_cast<spdlog::level::level_enum>(level));
}

void Log::SetClientLevel(Level level)
{
    ClientLogger->set_level(static_cast<spdlog::level::level_enum>(level));
}

void Log::Init(const std::string& clientName)
{
    spdlog::set_pattern("%^[%T.%f] %n: %v%$");

    CoreLogger = spdlog::stdout_color_mt("ENGINE");
    SetCoreLevel(Level::Trace);

    ClientLogger = spdlog::stdout_color_mt(clientName);
    SetClientLevel(Level::Trace);
}

} // namespace VoxelDynamics
