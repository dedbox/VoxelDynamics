#include <VoxelDynamics.hpp>

int main()
{
    const VoxelDynamics::Application::CreateInfo appInfo{
        .name      = "Sandbox",
        .title     = "Hello, world!",
        .width     = 1280,
        .height    = 720,
        .placement = VoxelDynamics::CenteredWindowPlacement(),
        .logLevel  = VoxelDynamics::Log::Level::Trace,
    };

    const auto app = VoxelDynamics::Application(appInfo);

    VoxelDynamics::Log::Trace("Hello, logs!");

    app.run();

    return 0;
}
