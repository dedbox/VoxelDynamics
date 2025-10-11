#include <VoxelDynamics.hpp>

// Application /////////////////////////////////////////////////////////////////////////////////////

using namespace VoxelDynamics;

class SandboxApp : public Application
{
public:
    explicit SandboxApp(const Application::BuildInfo& buildInfo)
        : Application(buildInfo)
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

public:
    class Builder : public Application::Builder
    {
    public:
        Builder()
        {
            _name      = "Sandbox";
            _placement = VoxelDynamics::CenteredWindowPlacement();
            _hidden    = true;
            _logLevel  = VoxelDynamics::Log::Level::Trace;
        }

        ~Builder() override = default;

        // allow moving
        Builder(Builder&&)            = default;
        Builder& operator=(Builder&&) = default;

        // allow copying
        Builder(const Builder&)            = default;
        Builder& operator=(const Builder&) = default;

        std::unique_ptr<Application> build() const override
        {
            return std::make_unique<SandboxApp>(GetBuildInfo());
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
