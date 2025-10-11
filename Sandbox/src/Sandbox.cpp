#include <VoxelDynamics.hpp>

// Sandbox App /////////////////////////////////////////////////////////////////////////////////////

using namespace VoxelDynamics;

class SandboxApp : public Application
{
public:
    explicit SandboxApp(const Application::BuildInfo& buildInfo)
        : Application(buildInfo)
        , _pipeline(_context.createGraphicsPipeline("shaders/slang.slang.spv"))
    {
        // connect event listeners
        Event::Bus::Connect<Event::WindowClose, &SandboxApp::onClose>(this);
        Event::Bus::Connect<Event::KeyDown, &SandboxApp::onKeyDown>(this);

        showWindow();
    }

    void onClose() { quit(); }

    void onKeyDown(const VoxelDynamics::Event::KeyDown& event)
    {

        switch (event.key)
        {
        case SDLK_ESCAPE:
            quit();
            return;

        default:
            break;
        }

        const auto keyName = [&]() -> std::string {
            const std::string name = SDL_GetKeyName(event.key);
            return !name.empty() ? name : std::format("<key {}>", event.key);
        }();

        VoxelDynamics::Log::Trace(
            "unhandled KeyDown event: {}{}", keyName, event.repeat ? " (repeat)" : "");
    }

private:
    Vulkan::Context::Pipeline _pipeline;
};

// Sandbox App Builder /////////////////////////////////////////////////////////////////////////////

class SanndboxAppBuilder : public ApplicationBuilder
{
public:
    SanndboxAppBuilder()
    {
        _name      = "Sandbox";
        _placement = VoxelDynamics::CenteredWindowPlacement();
        _hidden    = true;
        _logLevel  = VoxelDynamics::Log::Level::Debug;
    }

    // allow copying
    SanndboxAppBuilder(const SanndboxAppBuilder&)            = default;
    SanndboxAppBuilder& operator=(const SanndboxAppBuilder&) = default;

    // prevent moving
    SanndboxAppBuilder(SanndboxAppBuilder&&)            = delete;
    SanndboxAppBuilder& operator=(SanndboxAppBuilder&&) = delete;

    ~SanndboxAppBuilder() override = default;

    std::unique_ptr<Application> build() const override
    {
        return std::make_unique<SandboxApp>(GetBuildInfo());
    }
};

// Create Application //////////////////////////////////////////////////////////////////////////////

std::unique_ptr<VoxelDynamics::Application> VoxelDynamics::CreateApplication()
{
    auto app = SanndboxAppBuilder()
                   .version(0, 1, 0)
                   .title("Hello, world!")
                   .width(1280)
                   .height(720)
                   .build();
    return std::move(app);
}
