// Adapted from FlowCV b09c48e, GPL-3.0.
//
// Plugin CsvFile
//

#ifndef FLOWCV_PLUGIN_CSV_FILE_HPP_
#define FLOWCV_PLUGIN_CSV_FILE_HPP_
#include "output_support.hpp"
#include <chrono>
#include <fstream>
#include <filesystem>

namespace DSPatch::DSPatchables
{
namespace internal
{
class CsvFile;
}

class CsvFile : public FlowCV::Node
{
  public:
    CsvFile();
    std::shared_ptr<OutputUiBridge> UiHandle() const { return ui_; }
    std::string Status();

    ~CsvFile() override;
    std::string GetState() override;
    void SetState(std::string &&json_serialized) override;

  protected:
    void ApplyState_(const nlohmann::json& state, bool restoring);
    void Process_(SignalBus const &inputs, SignalBus &outputs) override;
    void OpenCsvFile(nlohmann::json &json_data);
    std::string GetRotateFilePath();

  protected:
    mutable std::recursive_mutex mutex_;
    std::shared_ptr<OutputUiBridge> ui_ = std::make_shared<OutputUiBridge>();
    std::string error_;
    std::unique_ptr<internal::CsvFile> p;
    bool start_new_file_;
    int counter_;
    int last_counter_;
    bool save_timestamp_;
    bool save_counter_;
    std::string csv_file_path_;
    int csv_cur_file_num_;
    std::ofstream csv_file_;
    bool show_file_dialog_;
    bool limit_file_size_;
    int max_file_size_kb_;
    bool rotating_files_;
    int num_files_;
};



}  // namespace DSPatch::DSPatchables
#endif  // FLOWCV_PLUGIN_CSV_FILE_HPP_
