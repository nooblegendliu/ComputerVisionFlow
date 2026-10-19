// Adapted from FlowCV Plugins/ImageWriter (GPL-3.0).
#include "image_writer.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <opencv2/imgcodecs.hpp>
#include <stdexcept>
#include <vector>

namespace DSPatch::DSPatchables {
ImageWriter::ImageWriter()
    : FlowCV::Node(ProcessOrder::OutOfOrder),
      state_(std::make_shared<internal::ImageWriterState>())
{
    SetComponentName_("Image_Writer");
    SetComponentCategory_(Category::Category_Output);
    SetComponentAuthor_("Richard");
    SetComponentVersion_("0.1.0");
    SetInputCount_(1, {"in"}, {IoType::Io_Type_CvMat});
    SetOutputCount_(0);
    SetEnabled(true);
}
void ImageWriter::SetOutputPath(const std::string& path) { state_->SetOutputPath(path); }
void ImageWriter::RequestWrite() { state_->RequestWrite(); }
ImageWriterStatus ImageWriter::GetWriteStatus() const { return state_->Snapshot(); }
std::string ImageWriter::GetState() { return "null"; }
void ImageWriter::SetState(std::string&& serialized)
{
    // Upstream accepts any valid JSON and restores no fields.
    const auto parsed = nlohmann::json::parse(serialized);
    (void)parsed;
}

void ImageWriter::Process_(SignalBus const& inputs, SignalBus&)
{
    const auto* frame = inputs.GetValue<cv::Mat>(0);
    if (!frame || frame->empty() || !IsEnabled()) return;
    // Preserve OutOfOrder scheduling while serializing a node's actual writes.
    std::lock_guard writeLock(writeMutex_);
    std::string path;
    {
        std::lock_guard lock(state_->mutex);
        if (!state_->status.pending) return;
        path = state_->status.outputPath;
        state_->status.pending = false;
    }
    bool written = false;
    std::string error;
    try {
        if (!path.empty()) {
            const auto nativePath = std::filesystem::u8path(path);
            std::vector<unsigned char> bytes;
            if (!cv::imencode(nativePath.extension().string(), *frame, bytes))
                throw std::runtime_error("Image encoding failed");
            // Native filesystem paths preserve UTF-8 names on Windows; image
            // encoding/options are the same as upstream imwrite defaults.
            std::ofstream stream(nativePath, std::ios::binary | std::ios::trunc);
            stream.write(reinterpret_cast<const char*>(bytes.data()),
                static_cast<std::streamsize>(bytes.size()));
            stream.close();
            if (!stream) throw std::runtime_error("Cannot write image output file");
            written = true;
        }
    }
    catch (const std::exception& exception) {
        error = exception.what();
        std::cerr << "Image_Writer: " << path << ": " << error << '\n';
    }
    {
        std::lock_guard lock(state_->mutex);
        ++state_->status.processedRequests;
        if (written) ++state_->status.completedWrites;
        state_->status.lastError = std::move(error);
    }
}

#ifndef IMAGE_WRITER_WITH_QT
EXPORT_PLUGIN(ImageWriter)
#endif
}
