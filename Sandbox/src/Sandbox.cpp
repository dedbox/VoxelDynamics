#include "VoxelDynamics/Config.hpp"
#include <VoxelDynamics.hpp>

int main()
{
    const VoxelDynamics::Application::CreateInfo appInfo{
        .contextInfo = {.appName = "Sandbox", .appVersion = VoxelDynamics::Version(0, 1, 0)},
        .title       = "Hello, world!",
        .width       = 1280,
        .height      = 720,
        .placement   = VoxelDynamics::CenteredWindowPlacement(),
        .logLevel    = VoxelDynamics::Log::Level::Trace,
    };

    VoxelDynamics::Application(appInfo).run();

    return 0;
}
