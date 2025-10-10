#include "VoxelDynamics/Core/Util.hpp"

namespace VoxelDynamics
{

std::vector<char> readFile(const std::string& fileName)
{
    std::ifstream file(fileName, std::ios::ate | std::ios::binary);
    if (!file.is_open())
        throw std::runtime_error(std::format("Could not open file `{}'", fileName));

    std::vector<char> buffer(file.tellg());

    file.seekg(0, std::ios::beg);
    file.read(buffer.data(), static_cast<std::streamsize>(buffer.size()));
    file.close();

    return buffer;
}

} // namespace VoxelDynamics
