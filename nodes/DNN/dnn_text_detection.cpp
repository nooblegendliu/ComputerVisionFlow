//
// Plugin DNN Text Detection
//

#include "dnn_text_detection.hpp"
#include <fstream>
#include <sstream>

using namespace DSPatch;
using namespace DSPatchables;



namespace DSPatch::DSPatchables
{

TextDetection::TextDetection() : FlowCV::Node(ProcessOrder::OutOfOrder)
{
    // Name and Category
    SetComponentName_("Text_Detection");
    SetComponentCategory_(DSPatch::Category::Category_DNN);
    SetComponentAuthor_("Richard");
    SetComponentVersion_("0.1.0");

    // 1 input
    SetInputCount_(1, {"in1"}, {IoType::Io_Type_CvMat});

    // 1 output
    SetOutputCount_(2, {"out", "json"}, {IoType::Io_Type_CvMat, IoType::Io_Type_JSON});

    // Set Defaults
    bbox_color_ = DnnColor(0.0f, 1.0f, 0.0f, 1.0f);
    bbox_thickness_ = 2.0;
    scale_ = 1.0f / 255.0f;
    mean_[0] = 122.67891434;
    mean_[1] = 116.66876762;
    mean_[2] = 104.00698793;
    bin_thresh_ = 0.3f;
    poly_thresh_ = 0.5f;
    unclip_ratio_ = 2.0f;
    max_candidates_ = 200;
    model_res_[0] = 736;
    model_res_[1] = 736;
    swap_rb_ = true;
    net_load_error = false;
    dnn_backend_idx_ = 0;
    current_backend_ = cv::dnn::DNN_BACKEND_DEFAULT;
    dnn_target_idx_ = 0;
    current_target_ = cv::dnn::DNN_TARGET_CPU;
    backend_list_ = dnn_backend_helper_.GetBackEndList();
    dnn_backend_helper_.UpdateTargetList((cv::dnn::Backend)dnn_backend_idx_);
    target_list_ = dnn_backend_helper_.GetTargetList();

    SetEnabled(true);
}

void TextDetection::InitDnn_()
{
    is_initialized_ = false;
    net_load_error = true;
    net_.reset();
    last_error_ = "Network initialization failed; check model and auxiliary paths.";

    try {
        net_ = std::make_unique<cv::dnn::TextDetectionModel_DB>(model_path_);
        net_->setPreferableBackend(current_backend_);
        net_->setPreferableTarget(current_target_);
        net_->setBinaryThreshold(bin_thresh_).setPolygonThreshold(poly_thresh_).setUnclipRatio(unclip_ratio_).setMaxCandidates(max_candidates_);
    }
    catch (...) {
        std::cerr << "DNN Initialization Error" << std::endl;
        net_load_error = true;
        is_initialized_ = false;
        return;
    }
    net_load_error = false;
    last_error_.clear();
    is_initialized_ = true;
    needs_reinit_ = false;
}

void TextDetection::Process_(SignalBus const &inputs, SignalBus &outputs)
{
    try {
    auto in1 = inputs.GetValue<cv::Mat>(0);
    if (!in1) {
        return;
    }

    std::lock_guard<std::recursive_mutex> lk(io_mutex_);
    if (!in1->empty()) {
        if (IsEnabled() && !net_load_error && is_initialized_) {
            last_error_.clear();
            // Process Image
            cv::Mat orig, frame;
            in1->copyTo(orig);
            cv::Size modelSize = cv::Size(model_res_[0], model_res_[1]);
            std::vector<std::vector<cv::Point>> results;

            if (pre_proc_resize_) {
                if (model_res_[0] > 0 && model_res_[1] > 0) {
                    cv::resize(*in1, frame, modelSize);
                }
                else
                    frame = orig;
            }
            else
                frame = orig;

            try {
                net_->setBinaryThreshold(bin_thresh_).setPolygonThreshold(poly_thresh_).setUnclipRatio(unclip_ratio_).setMaxCandidates(max_candidates_);
                net_->setInputParams(scale_, modelSize, cv::Scalar(mean_[0], mean_[1], mean_[2]), swap_rb_, false);
                net_->detect(frame, results);
            }
            catch (std::exception &e) {
                std::cerr << GetInstanceName() << ", Error Computing DNN" << std::endl;
                last_error_ = e.what();
                std::cerr << e.what() << std::endl;
                return;
            }

            nlohmann::json json_out;
            nlohmann::json detected;
            json_out["data_type"] = "objects";
            nlohmann::json ref;
            ref["w"] = orig.cols;
            ref["h"] = orig.rows;
            json_out["ref_frame"] = ref;

            for (const auto &res : results) {
                nlohmann::json jBbox;
                nlohmann::json cTmp;
                for (const auto &p : res) {
                    nlohmann::json jPoint;
                    if (pre_proc_resize_) {
                        float SX = (float)in1->cols / float(frame.cols);
                        float SY = (float)in1->rows / float(frame.rows);
                        jPoint["x"] = (int)((float)p.x * SX);
                        jPoint["y"] = (int)((float)p.y * SY);
                    }
                    else {
                        jPoint["x"] = p.x;
                        jPoint["y"] = p.y;
                    }
                    jBbox.emplace_back(jPoint);
                }
                cTmp["points"] = jBbox;
                detected.emplace_back(cTmp);
            }

            if (draw_detections_)
                polylines(frame, results, true, cv::Scalar(bbox_color_.z * 255, bbox_color_.y * 255, bbox_color_.x * 255), bbox_thickness_);

            if (pre_proc_resize_) {
                cv::resize(frame, orig, cv::Size(in1->cols, in1->rows));
            }

            if (!detected.empty())
                json_out["data"] = detected;
            outputs.SetValue(1, json_out);
            outputs.SetValue(0, orig);
        }
        else {
            outputs.SetValue(0, *in1);
        }
    }

    } catch (const std::exception& error) {
        std::lock_guard<std::recursive_mutex> errorLock(io_mutex_);
        last_error_ = error.what();
    }
}

std::string TextDetection::GetState()
{
    std::lock_guard<std::recursive_mutex> stateLock(io_mutex_);
    using namespace nlohmann;

    json state;

    state["model_path"] = model_path_;
    state["mean"] = {mean_[0], mean_[1], mean_[2]};
    state["scale"] = scale_;
    state["bin_thresh"] = bin_thresh_;
    state["poly_thresh"] = poly_thresh_;
    state["unclip_ratio"] = unclip_ratio_;
    state["max_candidates"] = max_candidates_;
    state["preproc"] = pre_proc_resize_;
    json jResize;
    jResize["w"] = model_res_[0];
    jResize["h"] = model_res_[1];
    state["model_res"] = jResize;
    if (draw_detections_) {
        state["bbox_thickness"] = bbox_thickness_;
        json tColor;
        tColor["R"] = bbox_color_.x;
        tColor["G"] = bbox_color_.y;
        tColor["B"] = bbox_color_.z;
        state["bbox_color"] = tColor;
    }
    state["swap_rb"] = swap_rb_;
    state["backend"] = (int)current_backend_;
    state["target"] = (int)current_target_;

    std::string stateSerialized = state.dump(4);

    return stateSerialized;
}

void TextDetection::SetState(std::string &&json_serialized)
{
    std::lock_guard<std::recursive_mutex> stateLock(io_mutex_);
    using namespace nlohmann;

    json state = json::parse(json_serialized);

    if (state.contains("model_path")) {
        model_path_ = state["model_path"].get<std::string>();
    }

    if (state.contains("bbox_thickness"))
        bbox_thickness_ = state["bbox_thickness"].get<int>();

    if (state.contains("bbox_color")) {
        bbox_color_.x = state["bbox_color"]["R"].get<float>();
        bbox_color_.y = state["bbox_color"]["G"].get<float>();
        bbox_color_.z = state["bbox_color"]["B"].get<float>();
        draw_detections_ = true;
    }

    if (state.contains("mean")) {
        mean_[0] = state["mean"][0].get<float>();
        mean_[1] = state["mean"][1].get<float>();
        mean_[2] = state["mean"][2].get<float>();
    }

    if (state.contains("model_res")) {
        model_res_[0] = state["model_res"]["w"].get<int>();
        model_res_[1] = state["model_res"]["h"].get<int>();
    }

    if (state.contains("preproc"))
        pre_proc_resize_ = state["preproc"].get<bool>();

    if (state.contains("scale"))
        scale_ = state["scale"].get<float>();

    if (state.contains("bin_thresh"))
        bin_thresh_ = state["bin_thresh"].get<float>();

    if (state.contains("poly_thresh"))
        poly_thresh_ = state["poly_thresh"].get<float>();

    if (state.contains("unclip_ratio"))
        unclip_ratio_ = state["unclip_ratio"].get<float>();

    if (state.contains("max_candidates"))
        max_candidates_ = state["max_candidates"].get<int>();

    if (state.contains("swap_rb"))
        swap_rb_ = state["swap_rb"].get<bool>();

    if (state.contains("backend"))
        current_backend_ = (cv::dnn::Backend)state["backend"].get<int>();

    if (state.contains("target"))
        current_target_ = (cv::dnn::Target)state["target"].get<int>();

    // Check Backend and Target
    backend_list_ = dnn_backend_helper_.GetBackEndList();
    bool backendMatch = false;
    for (int i = 0; i < backend_list_.size(); i++) {
        if (backend_list_[i].second == current_backend_) {
            backendMatch = true;
            dnn_backend_idx_ = i;
            bool targetMatch = false;
            dnn_backend_helper_.UpdateTargetList(backend_list_[i].second);
            target_list_ = dnn_backend_helper_.GetTargetList();
            for (int j = 0; j < target_list_.size(); j++) {
                if (target_list_[j].second == current_target_) {
                    targetMatch = true;
                    dnn_target_idx_ = j;
                    break;
                }
            }
            if (!targetMatch)
                current_target_ = cv::dnn::DNN_TARGET_CPU;
            break;
        }
    }
    if (!backendMatch) {
        current_backend_ = cv::dnn::DNN_BACKEND_DEFAULT;
        dnn_backend_idx_ = 0;
        dnn_backend_helper_.UpdateTargetList(current_backend_);
        target_list_ = dnn_backend_helper_.GetTargetList();
        current_target_ = cv::dnn::DNN_TARGET_CPU;
    }
    if (!target_list_.empty() && std::none_of(target_list_.begin(), target_list_.end(),
            [this](const auto& item) { return item.second == current_target_; }))
        current_target_ = target_list_.front().second;

    // Restore a configured network; ONNX does not require a separate config file.
    if (auto_initialize_) { is_initialized_ = false; net_.reset(); net_load_error = false; last_error_.clear(); }
    if (auto_initialize_ && !model_path_.empty()) {
        std::lock_guard<std::recursive_mutex> lk(io_mutex_);
        InitializeNetwork();
    }
}


nlohmann::json TextDetection::GetControlState() {
    std::lock_guard<std::recursive_mutex> lock(io_mutex_);
    auto state = nlohmann::json::parse(GetState());
    state["_draw"] = draw_detections_;
    state["bbox_color"] = {{"R", bbox_color_.x}, {"G", bbox_color_.y}, {"B", bbox_color_.z}};
    state["bbox_thickness"] = bbox_thickness_;
    return state;
}
void TextDetection::UpdateControlState(const nlohmann::json& patch) {
    std::lock_guard<std::recursive_mutex> lock(io_mutex_);
    auto state = GetControlState();
    state.merge_patch(patch);
    if (!state.at("_draw").get<bool>()) { state.erase("bbox_color"); state.erase("bbox_thickness"); }
    const bool reload = patch.contains("model_path") || patch.contains("config_path") || patch.contains("classes_path")
        || patch.contains("colors_path") || patch.contains("voc_path") || patch.contains("backend")
        || patch.contains("target") || patch.contains("_processing_mode") || patch.contains("model_res");
    auto_initialize_ = false;
    try { SetState(state.dump()); } catch (...) { auto_initialize_ = true; throw; }
    auto_initialize_ = true;
    // SetState intentionally preserves an existing drawing flag when its conditional
    // style is absent; the original GUI checkbox directly changes the flag.
    draw_detections_ = state.at("_draw").get<bool>();
    if (reload) { needs_reinit_ = true; }
}
bool TextDetection::InitializeNetwork() {
    std::lock_guard<std::recursive_mutex> lock(io_mutex_);
    try { InitDnn_(); } catch (const std::exception& error) { last_error_ = error.what(); net_load_error = true; is_initialized_ = false; net_.reset(); }
    return is_initialized_;
}
nlohmann::json TextDetection::GetNetworkStatus() {
    std::lock_guard<std::recursive_mutex> lock(io_mutex_);
    return {{"initialized", is_initialized_}, {"needs_reinit", needs_reinit_}, {"error", last_error_}};
}
std::vector<std::pair<std::string,int>> TextDetection::GetBackendOptions() {
    std::lock_guard<std::recursive_mutex> lock(io_mutex_);
    std::vector<std::pair<std::string,int>> result;
    for (const auto& entry: backend_list_) result.emplace_back(entry.first, static_cast<int>(entry.second));
    return result;
}
std::vector<std::pair<std::string,int>> TextDetection::GetTargetOptions() {
    std::lock_guard<std::recursive_mutex> lock(io_mutex_);
    std::vector<std::pair<std::string,int>> result;
    for (const auto& entry: target_list_) result.emplace_back(entry.first, static_cast<int>(entry.second));
    return result;
}

}  // End Namespace DSPatch::DSPatchables
