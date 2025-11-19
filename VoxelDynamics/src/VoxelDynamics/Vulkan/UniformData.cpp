#include "VoxelDynamics/Vulkan/UniformData.hpp"

namespace VoxelDynamics::Vulkan
{

UniformData::UniformData(
    const Context* context, std::optional<size_t> size, const std::string& debugName)
    : _context(context)
    , _uniformBuffer(createUniformBuffer(size, debugName))
{
}

std::optional<UniformBuffer> UniformData::createUniformBuffer(
    std::optional<size_t> size, const std::string& debugName) const
{
    if (!size.has_value())
        return std::nullopt;

    return _context->createUniformBuffer(*size, std::format("{} Uniform Buffer", debugName));
}

void UniformData::update(const void* data) const
{
    Log::Core::Assert(_uniformBuffer.has_value(), "tried to write to an unused uniform buffer");
    _uniformBuffer->update(data);
}

} // namespace VoxelDynamics::Vulkan
