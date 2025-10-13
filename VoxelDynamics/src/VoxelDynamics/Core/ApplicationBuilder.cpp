#include "VoxelDynamics/Core/ApplicationBuilder.hpp"

namespace VoxelDynamics
{

const Application::BuildInfo ApplicationBuilder::GetBuildInfo() const
{
    return Application::BuildInfo{
        // context
        .context =
            {
                .appName             = _name,
                .appVersion          = _version,
                .preferredDeviceType = _preferredDeviceType,
                .maxFramesInFlight   = _maxFramesInFlight,
            },
        // window
        .title     = _title.value_or(_name),
        .width     = _width,
        .height    = _height,
        .placement = _placement,
        .resizable = _resizable,
        .hidden    = _hidden,
        // application
        .logLevel   = _logLevel,
        .identifier = _identifier,
    };
}

std::unique_ptr<Application> ApplicationBuilder::build() const
{
    return std::make_unique<Application>(GetBuildInfo());
}

} // namespace VoxelDynamics
