#include "VoxelDynamics/Core/Log.hpp"

#include "minilog/minilog.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/spdlog.h"

namespace VoxelDynamics
{

std::shared_ptr<spdlog::logger> Log::CoreLogger;
std::shared_ptr<spdlog::logger> Log::ClientLogger;

void Log::Init(const std::string& clientName)
{
    spdlog::set_pattern("%^[%T.%f] %n: %v%$");

    CoreLogger = spdlog::stdout_color_mt("ENGINE");
    SetCoreLevel(Level::Info);

    ClientLogger = spdlog::stdout_color_mt(clientName);
    SetClientLevel(Level::Info);

    minilog::initialize(
        nullptr,
        {.logLevel               = minilog::Paranoid,
         .logLevelPrintToConsole = (minilog::eLogLevel)(5),
         .threadNames            = false});

    minilog::LogCallback cb = {.userData = CoreLogger.get()};
    cb.funcs[minilog::Log]  = [](void* data, const char* msg) {
        std::string s(msg);
        while (!s.empty() && (s.back() == '\n' || s.back() == '\r'))
            s.pop_back();
        s.erase(0, s.find_first_not_of("\n\r"));
        if (!s.empty())
            reinterpret_cast<spdlog::logger*>(data)->trace(s); // NOLINT
    };
    cb.funcs[minilog::Warning] = [](void* data, const char* msg) {
        std::string s(msg);
        while (!s.empty() && (s.back() == '\n' || s.back() == '\r'))
            s.pop_back();
        s.erase(0, s.find_first_not_of("\n\r"));
        if (!s.empty())
            reinterpret_cast<spdlog::logger*>(data)->warn(msg); // NOLINT
    };
    cb.funcs[minilog::FatalError] = [](void* data, const char* msg) {
        std::string s(msg);
        while (!s.empty() && (s.back() == '\n' || s.back() == '\r'))
            s.pop_back();
        s.erase(0, s.find_first_not_of("\n\r"));
        if (!s.empty())
            reinterpret_cast<spdlog::logger*>(data)->critical(msg); // NOLINT
    };
    minilog::callbackAdd(cb);
}

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

} // namespace VoxelDynamics
