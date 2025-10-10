#include <VoxelDynamics.hpp>

int main()
{
    VoxelDynamics::ApplicationBuilder()
        .name("Sandbox")
        .version(0, 1, 0)
        .title("Hello, world!")
        .width(1280)
        .height(720)
        .placement(VoxelDynamics::CenteredWindowPlacement())
        .logLevel(VoxelDynamics::Log::Level::Trace)
        .run();

    return 0;
}
