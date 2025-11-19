#pragma once

#include "VoxelDynamics/Vulkan/Context.hpp"

namespace VoxelDynamics::Vulkan
{

class UniformData
{
public:
    UniformData(const Context* context, std::optional<size_t> size, const std::string& debugName);

    ~UniformData() = default;

    // allow move
    UniformData(UniformData&&) noexcept            = default;
    UniformData& operator=(UniformData&&) noexcept = default;

    // prevent copy
    UniformData(const UniformData&)            = delete;
    UniformData& operator=(const UniformData&) = delete;

    // proxy dereference operator
    std::optional<UniformBuffer>& operator*() { return _uniformBuffer; }
    const std::optional<UniformBuffer>& operator*() const { return _uniformBuffer; }

    void update(const void* data) const;

private:
    const Context* _context;
    std::optional<UniformBuffer> _uniformBuffer;

    std::optional<UniformBuffer> createUniformBuffer(
        std::optional<size_t> size, const std::string& debugName) const;
};

} // namespace VoxelDynamics::Vulkan
