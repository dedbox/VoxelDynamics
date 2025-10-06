#pragma once

#include "spdlog/logger.h"

namespace VoxelDynamics
{

class Log
{
public:
    enum class Level : uint8_t
    {
        Trace    = spdlog::level::trace,
        Debug    = spdlog::level::debug,
        Info     = spdlog::level::info,
        Warn     = spdlog::level::warn,
        Error    = spdlog::level::err,
        Critical = spdlog::level::critical,
        Off      = spdlog::level::off,
    };

    static void Init(const std::string& clientName);

    static void SetLevel(Level level);
    static void SetCoreLevel(Level level);
    static void SetClientLevel(Level level);

private:
    static std::shared_ptr<spdlog::logger> CoreLogger;
    static std::shared_ptr<spdlog::logger> ClientLogger;

public:
    class Core
    {
    public:
        template <typename... Args>
        static void Trace(spdlog::format_string_t<Args...> fmt, Args&&... args)
        {
            if constexpr (is_logging_enabled)
                CoreLogger->trace(fmt, std::forward<Args>(args)...);
        }

        template <typename... Args>
        static void Debug(spdlog::format_string_t<Args...> fmt, Args&&... args)
        {
            if constexpr (is_logging_enabled)
                CoreLogger->debug(fmt, std::forward<Args>(args)...);
        }

        template <typename... Args>
        static void Info(spdlog::format_string_t<Args...> fmt, Args&&... args)
        {
            if constexpr (is_logging_enabled)
                CoreLogger->info(fmt, std::forward<Args>(args)...);
        }

        template <typename... Args>
        static void Warn(spdlog::format_string_t<Args...> fmt, Args&&... args)
        {
            if constexpr (is_logging_enabled)
                CoreLogger->warn(fmt, std::forward<Args>(args)...);
        }

        template <typename... Args>
        static void Error(spdlog::format_string_t<Args...> fmt, Args&&... args)
        {
            if constexpr (is_logging_enabled)
                CoreLogger->error(fmt, std::forward<Args>(args)...);
        }

        template <typename... Args>
        static void Critical(spdlog::format_string_t<Args...> fmt, Args&&... args)
        {
            if constexpr (is_logging_enabled)
                CoreLogger->critical(fmt, std::forward<Args>(args)...);
        }

        template <typename... Args>
        static void Assert(bool condition, spdlog::format_string_t<Args...> fmt, Args&&... args)
        {
            if constexpr (is_logging_enabled)
                if (!condition)
                {
                    Critical(fmt, std::forward<Args>(args)...);
                    __builtin_trap();
                }
        }

        static void AssertNoMsg(bool condition)
        {
            if constexpr (is_logging_enabled)
                if (!condition)
                {
                    Critical("Assertion failed");
                    __builtin_trap();
                }
        }
    };

    template <typename... Args>
    static void Trace(spdlog::format_string_t<Args...> fmt, Args&&... args)
    {
        if constexpr (is_logging_enabled)
            ClientLogger->trace(fmt, std::forward<Args>(args)...);
    }

    template <typename... Args>
    static void Debug(spdlog::format_string_t<Args...> fmt, Args&&... args)
    {
        if constexpr (is_logging_enabled)
            ClientLogger->debug(fmt, std::forward<Args>(args)...);
    }

    template <typename... Args>
    static void Info(spdlog::format_string_t<Args...> fmt, Args&&... args)
    {
        if constexpr (is_logging_enabled)
            ClientLogger->info(fmt, std::forward<Args>(args)...);
    }

    template <typename... Args>
    static void Warn(spdlog::format_string_t<Args...> fmt, Args&&... args)
    {
        if constexpr (is_logging_enabled)
            ClientLogger->warn(fmt, std::forward<Args>(args)...);
    }

    template <typename... Args>
    static void Error(spdlog::format_string_t<Args...> fmt, Args&&... args)
    {
        if constexpr (is_logging_enabled)
            ClientLogger->error(fmt, std::forward<Args>(args)...);
    }

    template <typename... Args>
    static void Critical(spdlog::format_string_t<Args...> fmt, Args&&... args)
    {
        if constexpr (is_logging_enabled)
            ClientLogger->critical(fmt, std::forward<Args>(args)...);
    }

    template <typename... Args>
    static void Assert(bool condition, spdlog::format_string_t<Args...> fmt, Args&&... args)
    {
        if constexpr (is_logging_enabled)
            if (!condition)
            {
                Critical(fmt, std::forward<Args>(args)...);
                __builtin_trap();
            }
    }

    static void AssertNoMsg(bool condition)
    {
        if constexpr (is_logging_enabled)
            if (!condition)
            {
                Critical("Assertion failed");
                __builtin_trap();
            }
    }
};

} // namespace VoxelDynamics
