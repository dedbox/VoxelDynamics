#pragma once

#include "VoxelDynamics/Core/Window.hpp"
#include "VoxelDynamics/Renderer/Vulkan/Context.hpp"

namespace VoxelDynamics
{

class Application
{
public:
    struct CreateInfo
    {
        std::string title;
        uint32_t width;
        uint32_t height;
        Vulkan::Context::CreateInfo context = {
            .appName             = title,
            .appVersion          = Version(1, 0, 0),
            .preferredDeviceType = Vulkan::DeviceType::Discrete,
        };
    };

    explicit Application(const CreateInfo& createInfo);

    ~Application();

    Application(const Application&)            = delete;
    Application(Application&&)                 = delete;
    Application& operator=(const Application&) = delete;
    Application& operator=(Application&&)      = delete;

    void run() const;

private:
    std::unique_ptr<Window> _window;
    std::unique_ptr<Vulkan::Context> _context;
};

} // namespace VoxelDynamics
