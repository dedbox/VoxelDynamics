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

std::string orstr(
    size_t n, size_t m = 1, const std::string& str1 = "", const std::string& str2 = "s");

} // namespace VoxelDynamics
