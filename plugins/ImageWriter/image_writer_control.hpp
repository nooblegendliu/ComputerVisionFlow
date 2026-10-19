// Adapted from FlowCV Plugins/ImageWriter (GPL-3.0).
#pragma once
#include <cstdint>
#include <string>

namespace DSPatch::DSPatchables {
struct ImageWriterStatus
{
    std::string outputPath;
    bool pending = false;
    std::uint64_t processedRequests = 0;
    std::uint64_t completedWrites = 0;
    std::string lastError;
};

// Optional, Qt-free controls contract across the .fp boundary. Runtime commands
// and the output path deliberately remain absent from upstream's null state.
class ImageWriterControl
{
public:
    virtual ~ImageWriterControl() = default;
    virtual void SetOutputPath(const std::string& utf8Path) = 0;
    virtual void RequestWrite() = 0;
    virtual ImageWriterStatus GetWriteStatus() const = 0;
};
}
