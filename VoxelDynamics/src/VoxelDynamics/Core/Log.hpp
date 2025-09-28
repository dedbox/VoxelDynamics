#pragma once

#include "spdlog/logger.h"

namespace VoxelDynamics
{

#ifndef ENABLE_LOGGING
inline constexpr bool is_logging_enabled = false;
#else
inline constexpr bool is_logging_enabled = true;
#endif

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

    static std::shared_ptr<spdlog::logger>& GetCore() { return CoreLogger; }
    static std::shared_ptr<spdlog::logger>& GetClient() { return ClientLogger; }

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
                GetCore()->trace(fmt, std::forward<Args>(args)...);
        }

        template <typename... Args>
        static void Debug(spdlog::format_string_t<Args...> fmt, Args&&... args)
        {
            if constexpr (is_logging_enabled)
                GetCore()->debug(fmt, std::forward<Args>(args)...);
        }

        template <typename... Args>
        static void Info(spdlog::format_string_t<Args...> fmt, Args&&... args)
        {
            if constexpr (is_logging_enabled)
                GetCore()->info(fmt, std::forward<Args>(args)...);
        }

        template <typename... Args>
        static void Warn(spdlog::format_string_t<Args...> fmt, Args&&... args)
        {
            if constexpr (is_logging_enabled)
                GetCore()->warn(fmt, std::forward<Args>(args)...);
        }

        template <typename... Args>
        static void Error(spdlog::format_string_t<Args...> fmt, Args&&... args)
        {
            if constexpr (is_logging_enabled)
                GetCore()->error(fmt, std::forward<Args>(args)...);
        }

        template <typename... Args>
        static void Critical(spdlog::format_string_t<Args...> fmt, Args&&... args)
        {
            if constexpr (is_logging_enabled)
                GetCore()->critical(fmt, std::forward<Args>(args)...);
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
    };

    template <typename... Args>
    static void Trace(spdlog::format_string_t<Args...> fmt, Args&&... args)
    {
        if constexpr (is_logging_enabled)
            GetClient()->trace(fmt, std::forward<Args>(args)...);
    }

    template <typename... Args>
    static void Debug(spdlog::format_string_t<Args...> fmt, Args&&... args)
    {
        if constexpr (is_logging_enabled)
            GetClient()->debug(fmt, std::forward<Args>(args)...);
    }

    template <typename... Args>
    static void Info(spdlog::format_string_t<Args...> fmt, Args&&... args)
    {
        if constexpr (is_logging_enabled)
            GetClient()->info(fmt, std::forward<Args>(args)...);
    }

    template <typename... Args>
    static void Warn(spdlog::format_string_t<Args...> fmt, Args&&... args)
    {
        if constexpr (is_logging_enabled)
            GetClient()->warn(fmt, std::forward<Args>(args)...);
    }

    template <typename... Args>
    static void Error(spdlog::format_string_t<Args...> fmt, Args&&... args)
    {
        if constexpr (is_logging_enabled)
            GetClient()->error(fmt, std::forward<Args>(args)...);
    }

    template <typename... Args>
    static void Critical(spdlog::format_string_t<Args...> fmt, Args&&... args)
    {
        if constexpr (is_logging_enabled)
            GetClient()->critical(fmt, std::forward<Args>(args)...);
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
};

} // namespace VoxelDynamics
