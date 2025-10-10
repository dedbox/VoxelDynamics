#include <VoxelDynamics.hpp>

// Application /////////////////////////////////////////////////////////////////////////////////////

class SandboxApp : public VoxelDynamics::Application
{
public:
    class Builder : public Application::Builder
    {
    public:
        Builder()
        {
            _name      = "Sandbox";
            _placement = VoxelDynamics::CenteredWindowPlacement();
            _logLevel  = VoxelDynamics::Log::Level::Trace;
        }
    };
};

// Sandbox /////////////////////////////////////////////////////////////////////////////////////////

std::unique_ptr<VoxelDynamics::Application> VoxelDynamics::CreateApplication()
{
    auto app = SandboxApp::Builder()
                   .version(0, 1, 0)
                   .title("Hello, world!")
                   .width(1280)
                   .height(720)
                   .build();
    return std::move(app);
}
