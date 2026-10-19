//
// Plugin DNN Text Detection
//

#ifndef FLOWCV_PLUGIN_DNN_TEXT_DETECTION_HPP_
#define FLOWCV_PLUGIN_DNN_TEXT_DETECTION_HPP_
#include <flowNode.h>
#include <opencv2/dnn.hpp>
#include <opencv2/imgproc.hpp>
#include <mutex>
#include <memory>
#include "dnn_controls.hpp"
#include "dnn_backend_helper.hpp"

namespace DSPatch::DSPatchables
{
class TextDetection final : public FlowCV::Node, public DnnControlInterface
{
  public:
    TextDetection();
    nlohmann::json GetControlState() override;
    void UpdateControlState(const nlohmann::json& patch) override;
    bool InitializeNetwork() override;
    nlohmann::json GetNetworkStatus() override;
    std::vector<std::pair<std::string, int>> GetBackendOptions() override;
    std::vector<std::pair<std::string, int>> GetTargetOptions() override;
    std::string GetState() override;
    void SetState(std::string &&json_serialized) override;

  protected:
    void Process_(SignalBus const &inputs, SignalBus &outputs) override;
    void InitDnn_();

  private:
    DnnBackendListHelper dnn_backend_helper_;
    bool is_initialized_{};
    bool pre_proc_resize_{};
    std::recursive_mutex io_mutex_;
    bool auto_initialize_ = true;
    std::string last_error_;
    std::string model_path_;
    std::vector<std::pair<std::string, cv::dnn::Backend>> backend_list_;
    std::vector<std::pair<std::string, cv::dnn::Target>> target_list_;
    int dnn_backend_idx_;
    cv::dnn::Backend current_backend_;
    int dnn_target_idx_;
    cv::dnn::Target current_target_;
    std::unique_ptr<cv::dnn::TextDetectionModel_DB> net_;
    DnnColor bbox_color_;
    int bbox_thickness_;
    float mean_[3]{};
    float scale_;
    float bin_thresh_;
    float poly_thresh_;
    float unclip_ratio_;
    int max_candidates_;
    int model_res_[2]{};
    bool net_load_error{};
    bool needs_reinit_{};
    bool swap_rb_{};
    bool draw_detections_{};
};

}  // namespace DSPatch::DSPatchables

#endif  // FLOWCV_PLUGIN_DNN_TEXT_DETECTION_HPP_
