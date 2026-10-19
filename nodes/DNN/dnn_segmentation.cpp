//
// Plugin DNN Classification
//

#include "dnn_segmentation.hpp"
#include <fstream>
#include <sstream>

using namespace DSPatch;
using namespace DSPatchables;



namespace DSPatch::DSPatchables
{

Segmentation::Segmentation() : FlowCV::Node(ProcessOrder::OutOfOrder)
{
    // Name and Category
    SetComponentName_("Segmentation");
    SetComponentCategory_(DSPatch::Category::Category_DNN);
    SetComponentAuthor_("Richard");
    SetComponentVersion_("0.1.0");

    // 1 input
    SetInputCount_(1, {"in1"}, {IoType::Io_Type_CvMat});

    // 1 output
    SetOutputCount_(2, {"out", "json"}, {IoType::Io_Type_CvMat, IoType::Io_Type_JSON});

    // Set Defaults
    scale_ = 1.0f;
    model_res_[0] = 224;
    model_res_[1] = 224;
    swap_rb_ = false;
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

void Segmentation::InitDnn_()
{
    is_initialized_ = false;
    net_load_error = true;
    net_.reset();
    last_error_ = "Network initialization failed; check model and auxiliary paths.";
    // Load Classes list
    class_list_.clear();
    std::ifstream ifsClasses(classes_path_);
    if (!ifsClasses.is_open()) {
        std::cerr << GetInstanceName() << ", File " + classes_path_ + " not found" << std::endl;
        return;
    }
    std::string line;
    while (std::getline(ifsClasses, line)) {
        class_list_.emplace_back(line);
    }

    // Open file with colors.
    colors_list_.clear();
    if (!colors_path_.empty()) {
        std::ifstream ifsColors(colors_path_);
        if (!ifsColors.is_open()) {
            std::cerr << GetInstanceName() << ", File " + colors_path_ + " not found" << std::endl;
            return;
        }
        while (std::getline(ifsColors, line)) {
            std::stringstream colorStr(line);
            cv::Vec3b color(0, 0, 0);
            std::string item;
            int i = 0;
            while (getline(colorStr, item, ' ')) {
                color[i] = std::stoi(item);
                i++;
                if (i > 2)
                    break;
            }
            std::cout << std::endl;
            colors_list_.push_back(color);
        }
    }

    try {
        if (!config_path_.empty())
            net_ = std::make_unique<cv::dnn::Net>(cv::dnn::readNet(model_path_, config_path_, ""));
        else
            net_ = std::make_unique<cv::dnn::Net>(cv::dnn::readNet(model_path_, "", ""));
        net_->setPreferableBackend(current_backend_);
        net_->setPreferableTarget(current_target_);
        out_names_ = net_->getUnconnectedOutLayersNames();
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

void Segmentation::ColorizeSegmentation_(const cv::Mat &score, cv::Mat &segm)
{
    CV_Assert(score.dims == 4 && score.size[0] == 1 && score.size[1] > 0 && score.size[1] <= 256 && score.depth() == CV_32F);
    const int rows = score.size[2];
    const int cols = score.size[3];
    const int chns = score.size[1];

    if (colors_list_.empty()) {
        // Generate colors.
        colors_list_.emplace_back();
        for (int i = 1; i < chns; ++i) {
            cv::Vec3b color;
            for (int j = 0; j < 3; ++j)
                color[j] = (colors_list_[i - 1][j] + rand() % 256) / 2;
            colors_list_.emplace_back(color);
        }
    }
    else if (chns != (int)colors_list_.size()) {
        std::cerr << cv::format("Number of output classes doesn't match number of colors (%d != %zu)", chns, colors_list_.size()) << std::endl;
        return;
    }

    cv::Mat maxCl = cv::Mat::zeros(rows, cols, CV_8UC1);
    cv::Mat maxVal(rows, cols, CV_32FC1, score.data);
    for (int ch = 1; ch < chns; ch++) {
        for (int row = 0; row < rows; row++) {
            const auto *ptrScore = score.ptr<float>(0, ch, row);
            auto *ptrMaxCl = maxCl.ptr<uint8_t>(row);
            auto *ptrMaxVal = maxVal.ptr<float>(row);
            for (int col = 0; col < cols; col++) {
                if (ptrScore[col] > ptrMaxVal[col]) {
                    ptrMaxVal[col] = ptrScore[col];
                    ptrMaxCl[col] = (uchar)ch;
                }
            }
        }
    }

    segm.create(rows, cols, CV_8UC3);
    for (int row = 0; row < rows; row++) {
        const uchar *ptrMaxCl = maxCl.ptr<uchar>(row);
        auto *ptrSegm = segm.ptr<cv::Vec3b>(row);
        for (int col = 0; col < cols; col++) {
            ptrSegm[col] = colors_list_[ptrMaxCl[col]];
        }
    }
}

void Segmentation::Process_(SignalBus const &inputs, SignalBus &outputs)
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
            cv::Mat orig, frame, blob, prob;
            in1->copyTo(orig);
            cv::Size modelSize = cv::Size(model_res_[0], model_res_[1]);

            if (pre_proc_resize_) {
                if (model_res_[0] > 0 && model_res_[1] > 0) {
                    cv::resize(*in1, frame, modelSize);
                }
                else
                    frame = orig;
            }
            else
                frame = orig;

            nlohmann::json json_out;
            nlohmann::json detected;
            json_out["data_type"] = "segmentation";
            nlohmann::json ref;
            ref["w"] = orig.cols;
            ref["h"] = orig.rows;
            json_out["ref_frame"] = ref;

            try {
                cv::dnn::blobFromImage(frame, blob, scale_, modelSize, cv::Scalar(mean_[0], mean_[1], mean_[2]), swap_rb_, false);
                net_->setInput(blob);
                cv::Mat score = net_->forward();
                cv::Mat segm;
                ColorizeSegmentation_(score, segm);
                resize(segm, frame, orig.size(), 0, 0, cv::INTER_NEAREST);
            }
            catch (std::exception &e) {
                std::cerr << GetInstanceName() << ", Error Computing DNN" << std::endl;
                last_error_ = e.what();
                std::cerr << e.what() << std::endl;
                return;
            }
            if (colors_list_.size() == class_list_.size()) {
                for (int i = 0; i < colors_list_.size(); i++) {
                    nlohmann::json cTmp;
                    cTmp["name"] = class_list_.at(i);
                    nlohmann::json jColor;
                    jColor["b"] = colors_list_.at(i)[0];
                    jColor["g"] = colors_list_.at(i)[1];
                    jColor["r"] = colors_list_.at(i)[2];
                    cTmp["color"] = jColor;
                    detected.emplace_back(cTmp);
                }
            }

            if (!detected.empty())
                json_out["data"] = detected;
            outputs.SetValue(1, json_out);
            outputs.SetValue(0, frame);
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

std::string Segmentation::GetState()
{
    std::lock_guard<std::recursive_mutex> stateLock(io_mutex_);
    using namespace nlohmann;

    json state;

    state["model_path"] = model_path_;
    state["config_path"] = config_path_;
    state["classes_path"] = classes_path_;
    state["colors_path"] = colors_path_;
    state["mean"] = {mean_[0], mean_[1], mean_[2]};
    state["scale"] = scale_;
    state["preproc"] = pre_proc_resize_;
    json jResize;
    jResize["w"] = model_res_[0];
    jResize["h"] = model_res_[1];
    state["model_res"] = jResize;
    state["swap_rb"] = swap_rb_;
    state["backend"] = (int)current_backend_;
    state["target"] = (int)current_target_;

    std::string stateSerialized = state.dump(4);

    return stateSerialized;
}

void Segmentation::SetState(std::string &&json_serialized)
{
    std::lock_guard<std::recursive_mutex> stateLock(io_mutex_);
    using namespace nlohmann;

    json state = json::parse(json_serialized);

    if (state.contains("model_path")) {
        model_path_ = state["model_path"].get<std::string>();
    }
    if (state.contains("config_path")) {
        config_path_ = state["config_path"].get<std::string>();
    }
    if (state.contains("classes_path")) {
        classes_path_ = state["classes_path"].get<std::string>();
    }
    if (state.contains("colors_path")) {
        colors_path_ = state["colors_path"].get<std::string>();
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
    if (auto_initialize_ && !model_path_.empty() && !classes_path_.empty()) {
        std::lock_guard<std::recursive_mutex> lk(io_mutex_);
        InitializeNetwork();
    }
}


nlohmann::json Segmentation::GetControlState() {
    std::lock_guard<std::recursive_mutex> lock(io_mutex_);
    auto state = nlohmann::json::parse(GetState());
    return state;
}
void Segmentation::UpdateControlState(const nlohmann::json& patch) {
    std::lock_guard<std::recursive_mutex> lock(io_mutex_);
    auto state = GetControlState();
    state.merge_patch(patch);
    const bool reload = patch.contains("model_path") || patch.contains("config_path") || patch.contains("classes_path")
        || patch.contains("colors_path") || patch.contains("voc_path") || patch.contains("backend")
        || patch.contains("target") || patch.contains("_processing_mode") || patch.contains("model_res");
    auto_initialize_ = false;
    try { SetState(state.dump()); } catch (...) { auto_initialize_ = true; throw; }
    auto_initialize_ = true;
    if (reload) { needs_reinit_ = true; }
}
bool Segmentation::InitializeNetwork() {
    std::lock_guard<std::recursive_mutex> lock(io_mutex_);
    try { InitDnn_(); } catch (const std::exception& error) { last_error_ = error.what(); net_load_error = true; is_initialized_ = false; net_.reset(); }
    return is_initialized_;
}
nlohmann::json Segmentation::GetNetworkStatus() {
    std::lock_guard<std::recursive_mutex> lock(io_mutex_);
    return {{"initialized", is_initialized_}, {"needs_reinit", needs_reinit_}, {"error", last_error_}};
}
std::vector<std::pair<std::string,int>> Segmentation::GetBackendOptions() {
    std::lock_guard<std::recursive_mutex> lock(io_mutex_);
    std::vector<std::pair<std::string,int>> result;
    for (const auto& entry: backend_list_) result.emplace_back(entry.first, static_cast<int>(entry.second));
    return result;
}
std::vector<std::pair<std::string,int>> Segmentation::GetTargetOptions() {
    std::lock_guard<std::recursive_mutex> lock(io_mutex_);
    std::vector<std::pair<std::string,int>> result;
    for (const auto& entry: target_list_) result.emplace_back(entry.first, static_cast<int>(entry.second));
    return result;
}

}  // End Namespace DSPatch::DSPatchables
