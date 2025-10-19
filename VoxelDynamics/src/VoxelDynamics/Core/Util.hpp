#pragma once

namespace VoxelDynamics
{

// variant matching
template <class... Ts>
struct overloaded : Ts...
{
    using Ts::operator()...;
};

std::vector<char> readFile(const std::string& fileName);

} // namespace VoxelDynamics
