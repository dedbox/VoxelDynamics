#pragma once

#include "VoxelDynamics/Core/Application.hpp"

namespace VoxelDynamics
{

class ApplicationBuilder
{
public:
    ApplicationBuilder() = default;

    // allow copying
    ApplicationBuilder(const ApplicationBuilder&)            = default;
    ApplicationBuilder& operator=(const ApplicationBuilder&) = default;

    // prevent moving
    ApplicationBuilder(ApplicationBuilder&&)            = delete;
    ApplicationBuilder& operator=(ApplicationBuilder&&) = delete;

    virtual ~ApplicationBuilder() = default;

    const Application::BuildInfo GetBuildInfo() const;
    virtual std::unique_ptr<Application> build() const;

    // application
    // ---------------------------------------------------------------------------------

    ApplicationBuilder& logLevel(Log::Level level)
    {
        _logLevel = level;
        return *this;
    }

    ApplicationBuilder& identifier(const std::string& identifier)
    {
        _identifier = identifier;
        return *this;
    }

    // window
    // --------------------------------------------------------------------------------------

    ApplicationBuilder& title(const std::string& title)
    {
        _title = title;
        return *this;
    }

    ApplicationBuilder& width(uint32_t width)
    {
        _width = width;
        return *this;
    }

    ApplicationBuilder& height(uint32_t height)
    {
        _height = height;
        return *this;
    }

    ApplicationBuilder& placement(WindowPlacement placement)
    {
        _placement = placement;
        return *this;
    }

    ApplicationBuilder& resizable(bool resizable)
    {
        _resizable = resizable;
        return *this;
    }

    ApplicationBuilder& hidden(bool hidden)
    {
        _hidden = hidden;
        return *this;
    }

    // context
    // -------------------------------------------------------------------------------------

    ApplicationBuilder& name(const std::string& name)
    {
        _name = name;
        return *this;
    }

    ApplicationBuilder& version(uint32_t major, uint32_t minor, uint32_t patch)
    {
        _version = Version(major, minor, patch);
        return *this;
    }

    ApplicationBuilder& preferredDeviceType(vk::PhysicalDeviceType type)
    {
        _preferredDeviceType = type;
        return *this;
    }

    // =============================================================================================

protected:
    // window
    std::optional<std::string> _title = std::nullopt;
    uint32_t _width                   = 800;
    uint32_t _height                  = 600;
    bool _resizable                   = false;
    WindowPlacement _placement        = DefaultWindowPlacement();
    bool _hidden                      = false;

    // context
    std::string _name                           = "VxD-App";
    uint64_t _version                           = Version(1, 0, 0);
    vk::PhysicalDeviceType _preferredDeviceType = vk::PhysicalDeviceType::eDiscreteGpu;

    // application
    Log::Level _logLevel    = Log::Level::Info;
    std::string _identifier = std::format("com.voxeldynamics.{}", _name);
};

} // namespace VoxelDynamics
