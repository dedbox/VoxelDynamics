#pragma once

#include "glslang/Public/ShaderLang.h"

namespace VoxelDynamics
{

// from
// https://github.com/KhronosGroup/glslang/blob/a57276bf558f5cf94d3a9854ebdf5a2236849a5a/StandAlone/DirStackFileIncluder.h

/** Default include class for normal include convention of search backward through the stack of
 * active include paths (for nested includes).
 *
 * Can be overridden to customize.
 */
class DirStackFileIncluder : public glslang::TShader::Includer
{
public:
    DirStackFileIncluder() = default;

    DirStackFileIncluder(
        std::vector<std::string> directoryStack, std::set<std::string> includedFiles)
        : _directoryStack(std::move(directoryStack))
        , _includedFiles(std::move(includedFiles))
    {
    }

    DirStackFileIncluder(const DirStackFileIncluder&)            = default;
    DirStackFileIncluder& operator=(const DirStackFileIncluder&) = default;

    DirStackFileIncluder(DirStackFileIncluder&&)            = default;
    DirStackFileIncluder& operator=(DirStackFileIncluder&&) = default;

    IncludeResult* includeLocal(
        const char* headerName, const char* includerName, size_t inclusionDepth) override
    {
        return readLocalPath(headerName, includerName, static_cast<int>(inclusionDepth));
    }

    IncludeResult* includeSystem(
        const char* headerName, const char* /*includerName*/, size_t /*inclusionDepth*/) override
    {
        return readSystemPath(headerName);
    }

    /** Externally set directories. E.g., from a command-line -I<dir>.
     *
     *  - Most-recently pushed are checked first.
     *  - All these are checked after the parse-time stack of local directories
     *    is checked.
     *  - This only applies to the "local" form of #include.
     *  - Makes its own copy of the path.
     */
    virtual void pushExternalLocalDirectory(const std::string& dir)
    {
        _directoryStack.push_back(dir);
        _externalLocalDirectoryCount = static_cast<int>(_directoryStack.size());
    }

    void releaseInclude(IncludeResult* result) override
    {
        if (result != nullptr)
        {
            delete[] static_cast<char*>(result->userData); // NOLINT
            delete result;                                 // NOLINT
        }
    }

    virtual std::set<std::string> getIncludedFiles() { return _includedFiles; }

    ~DirStackFileIncluder() override = default;

protected:
    std::vector<std::string> _directoryStack;
    int _externalLocalDirectoryCount{0};
    std::set<std::string> _includedFiles;

    /** Search for a valid "local" path based on combining the stack of include directories and the
     * nominal name of the header.
     */
    virtual IncludeResult* readLocalPath(
        const char* headerName, const char* includerName, int depth)
    {
        // Discard popped include directories, and initialize when at parse-time first level.
        _directoryStack.resize(depth + _externalLocalDirectoryCount);
        if (depth == 1)
            _directoryStack.back() = getDirectory(includerName);

        // Find a directory that works, using a reverse search of the include stack.
        for (const auto& dir : std::views::reverse(_directoryStack))
        {
            std::string path = dir + '/' + headerName;
            std::ranges::replace(path, '\\', '/');
            std::ifstream file(path, std::ios_base::binary | std::ios_base::ate);
            if (file)
            {
                _directoryStack.push_back(getDirectory(path));
                _includedFiles.insert(path);
                return newIncludeResult(path, file, static_cast<int>(file.tellg()));
            }
        }

        return nullptr;
    }

    /** Search for a valid <system> path.
     *
     * Not implemented yet; returning nullptr signals failure to find.
     */
    virtual IncludeResult* readSystemPath(const char* /*headerName*/) const { return nullptr; }

    /** Do actual reading of the file, filling in a new include result. */
    virtual IncludeResult* newIncludeResult(
        const std::string& path, std::ifstream& file, int length) const
    {
        char* content = new char[length]; // NOLINT
        file.seekg(0, file.beg);
        file.read(content, length);
        return new IncludeResult(path, content, length, content); // NOLINT
    }

    /** If no path markers, return current working directory. Otherwise, strip file name and return
     * path leading up to it.
     */
    virtual std::string getDirectory(const std::string& path) const
    {
        size_t last = path.find_last_of("/\\");
        return last == std::string::npos ? "." : path.substr(0, last);
    }
};

} // namespace VoxelDynamics
