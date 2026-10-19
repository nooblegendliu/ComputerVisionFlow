// Adapted from FlowCV Plugins/ImageWriter (GPL-3.0).
#pragma once
#include "image_writer_control.hpp"
#include <flowNode.h>
#include <memory>
#include <mutex>

namespace DSPatch::DSPatchables {
namespace internal {
struct ImageWriterState
{
    mutable std::mutex mutex;
    ImageWriterStatus status;
    void SetOutputPath(const std::string& path)
    {
        std::lock_guard lock(mutex);
        status.outputPath = path;
    }
    void RequestWrite()
    {
        std::lock_guard lock(mutex);
        status.pending = true;
    }
    ImageWriterStatus Snapshot() const
    {
        std::lock_guard lock(mutex);
        return status;
    }
};
}

class ImageWriter : public FlowCV::Node, public ImageWriterControl
{
public:
    ImageWriter();
    void SetOutputPath(const std::string& utf8Path) override;
    void RequestWrite() override;
    ImageWriterStatus GetWriteStatus() const override;
    std::string GetState() override;
    void SetState(std::string&& serialized) override;
protected:
    void Process_(SignalBus const& inputs, SignalBus& outputs) override;
    std::shared_ptr<internal::ImageWriterState> state_;
private:
    std::mutex writeMutex_;
};
}
