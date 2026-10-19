//
// Plugin DNN Text Recognition
//

#include "dnn_text_recognition.hpp"
#include <fstream>
#include <sstream>

using namespace DSPatch;
using namespace DSPatchables;



void fourPointsTransform(const cv::Mat &frame, const std::vector<cv::Point2f> &vertices, cv::Mat &result)
{
    CV_Assert(vertices.size() == 4);
    const cv::Size outputSize = cv::Size(100, 32);

    cv::Point2f targetVertices[4] = {
        cv::Point(0, outputSize.height - 1), cv::Point(0, 0), cv::Point(outputSize.width - 1, 0), cv::Point(outputSize.width - 1, outputSize.height - 1)};
    cv::Mat rotationMatrix = getPerspectiveTransform(vertices.data(), targetVertices);

    warpPerspective(frame, result, rotationMatrix, outputSize);
}

namespace DSPatch::DSPatchables
{

TextRecognition::TextRecognition() : FlowCV::Node(ProcessOrder::OutOfOrder)
{
    // Name and Category
    SetComponentName_("Text_Recognition");
    SetComponentCategory_(DSPatch::Category::Category_DNN);
    SetComponentAuthor_("Richard");
    SetComponentVersion_("0.1.0");

    // 1 input
    SetInputCount_(2, {"in1", "json"}, {IoType::Io_Type_CvMat, IoType::Io_Type_JSON});

    // 1 output
    SetOutputCount_(2, {"out", "json"}, {IoType::Io_Type_CvMat, IoType::Io_Type_JSON});

    // Set Defaults
    text_pos_ = cv::Point2i(0, 0);
    text_scale_ = 0.5;
    text_color_ = DnnColor(0.0f, 1.0f, 0.0f, 1.0f);
    text_thickness_ = 2.0;
    scale_ = 1.0f / 127.5;
    mean_[0] = 127.5;
    mean_[1] = 127.5;
    mean_[2] = 127.5;
    model_res_[0] = 100;
    model_res_[1] = 32;
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

void TextRecognition::InitDnn_()
{
    is_initialized_ = false;
    net_load_error = true;
    net_.reset();
    last_error_ = "Network initialization failed; check model and auxiliary paths.";
    // Load Vocabulary list
    vocabulary_.clear();
    if (!voc_path_.empty()) {
        std::ifstream ifs(voc_path_);
        if (!ifs.is_open()) {
            std::cerr << GetInstanceName() << ", File " + voc_path_ + " not found" << std::endl;
            return;
        }
        else {
            std::string line;
            while (std::getline(ifs, line)) {
                vocabulary_.emplace_back(line);
            }
        }
    }

    try {
        net_ = std::make_unique<cv::dnn::TextRecognitionModel>(model_path_);
        net_->setPreferableBackend(current_backend_);
        net_->setPreferableTarget(current_target_);
        net_->setVocabulary(vocabulary_);
        net_->setDecodeType("CTC-greedy");
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

void TextRecognition::Process_(SignalBus const &inputs, SignalBus &outputs)
{
    try {
    auto in1 = inputs.GetValue<cv::Mat>(0);
    auto in_json = inputs.GetValue<nlohmann::json>(1);
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
            json_out["data_type"] = "text";
            nlohmann::json ref;
            ref["w"] = orig.cols;
            ref["h"] = orig.rows;
            json_out["ref_frame"] = ref;

            try {
                net_->setInputParams(scale_, modelSize, cv::Scalar(mean_[0], mean_[1], mean_[2]));
                cv::Mat recInput = frame;
                if (in_json) {
                    nlohmann::json json_data = *in_json;
                    if (!json_data.empty()) {
                        bool hasObjects = false;
                        if (json_data.contains("data_type")) {
                            if (json_data["data_type"].get<std::string>() == "objects") {
                                if (json_data.contains("data")) {
                                    if (!json_data["data"].empty())
                                        hasObjects = true;
                                }
                            }
                        }
                        if (hasObjects) {
                            for (const auto &d : json_data["data"]) {
                                if (d.contains("points")) {
                                    std::vector<cv::Point2f> quadrangle_2f;
                                    for (const auto &bb : d["points"]) {
                                        cv::Point2f pt;
                                        pt.x = bb["x"].get<float>();
                                        pt.y = bb["y"].get<float>();
                                        quadrangle_2f.emplace_back(pt);
                                    }
                                    cv::Mat cropped;
                                    fourPointsTransform(recInput, quadrangle_2f, cropped);
                                    net_->setInputParams(scale_, modelSize, cv::Scalar(mean_[0], mean_[1], mean_[2]), swap_rb_, false);
                                    std::string recognitionResult = net_->recognize(cropped);
                                    nlohmann::json cTmp;
                                    nlohmann::json jBbox;
                                    cv::Rect bbox = cv::boundingRect(quadrangle_2f);
                                    jBbox["x"] = bbox.x;
                                    jBbox["y"] = bbox.y;
                                    jBbox["w"] = bbox.width;
                                    jBbox["h"] = bbox.height;
                                    cTmp["bbox"] = jBbox;
                                    cTmp["text"] = recognitionResult;
                                    detected.emplace_back(cTmp);
                                    if (draw_text_) {
                                        cv::putText(frame, recognitionResult,
                                            cv::Point((int)quadrangle_2f.at(0).x + text_pos_.x, (int)quadrangle_2f.at(0).y + text_pos_.y),
                                            cv::FONT_HERSHEY_SIMPLEX, text_scale_, cv::Scalar(text_color_.z * 255, text_color_.y * 255, text_color_.x * 255),
                                            text_thickness_);
                                    }
                                }
                            }
                        }
                    }
                }
            }
            catch (std::exception &e) {
                std::cerr << GetInstanceName() << ", Error Computing DNN" << std::endl;
                last_error_ = e.what();
                std::cerr << e.what() << std::endl;
                return;
            }

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

std::string TextRecognition::GetState()
{
    std::lock_guard<std::recursive_mutex> stateLock(io_mutex_);
    using namespace nlohmann;

    json state;

    state["model_path"] = model_path_;
    state["voc_path"] = voc_path_;
    state["mean"] = {mean_[0], mean_[1], mean_[2]};
    state["scale"] = scale_;
    state["preproc"] = pre_proc_resize_;
    json jResize;
    jResize["w"] = model_res_[0];
    jResize["h"] = model_res_[1];
    state["model_res"] = jResize;
    if (draw_text_) {
        json pos;
        pos["x"] = text_pos_.x;
        pos["y"] = text_pos_.y;
        state["txt_position"] = pos;
        state["txt_scale"] = text_scale_;
        state["txt_thickness"] = text_thickness_;
        json tColor;
        tColor["R"] = text_color_.x;
        tColor["G"] = text_color_.y;
        tColor["B"] = text_color_.z;
        state["txt_color"] = tColor;
    }
    state["swap_rb"] = swap_rb_;
    state["backend"] = (int)current_backend_;
    state["target"] = (int)current_target_;

    std::string stateSerialized = state.dump(4);

    return stateSerialized;
}

void TextRecognition::SetState(std::string &&json_serialized)
{
    std::lock_guard<std::recursive_mutex> stateLock(io_mutex_);
    using namespace nlohmann;

    json state = json::parse(json_serialized);

    if (state.contains("model_path")) {
        model_path_ = state["model_path"].get<std::string>();
    }

    if (state.contains("voc_path")) {
        voc_path_ = state["voc_path"].get<std::string>();
    }

    if (state.contains("txt_position")) {
        text_pos_.x = state["txt_position"]["x"].get<int>();
        text_pos_.y = state["txt_position"]["y"].get<int>();
    }
    if (state.contains("txt_scale"))
        text_scale_ = state["txt_scale"].get<float>();

    if (state.contains("txt_thickness"))
        text_thickness_ = state["txt_thickness"].get<int>();

    if (state.contains("txt_color")) {
        text_color_.x = state["txt_color"]["R"].get<float>();
        text_color_.y = state["txt_color"]["G"].get<float>();
        text_color_.z = state["txt_color"]["B"].get<float>();
        draw_text_ = true;
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
    if (auto_initialize_ && !model_path_.empty() && !voc_path_.empty()) {
        std::lock_guard<std::recursive_mutex> lk(io_mutex_);
        InitializeNetwork();
    }
}


nlohmann::json TextRecognition::GetControlState() {
    std::lock_guard<std::recursive_mutex> lock(io_mutex_);
    auto state = nlohmann::json::parse(GetState());
    state["_draw"] = draw_text_;
    state["txt_color"] = {{"R", text_color_.x}, {"G", text_color_.y}, {"B", text_color_.z}};
    state["txt_position"] = {{"x", text_pos_.x}, {"y", text_pos_.y}};
    state["txt_scale"] = text_scale_;
    state["txt_thickness"] = text_thickness_;
    return state;
}
void TextRecognition::UpdateControlState(const nlohmann::json& patch) {
    std::lock_guard<std::recursive_mutex> lock(io_mutex_);
    auto state = GetControlState();
    state.merge_patch(patch);
    if (!state.at("_draw").get<bool>()) { state.erase("txt_color"); state.erase("txt_position"); state.erase("txt_scale"); state.erase("txt_thickness"); }
    const bool reload = patch.contains("model_path") || patch.contains("config_path") || patch.contains("classes_path")
        || patch.contains("colors_path") || patch.contains("voc_path") || patch.contains("backend")
        || patch.contains("target") || patch.contains("_processing_mode") || patch.contains("model_res");
    auto_initialize_ = false;
    try { SetState(state.dump()); } catch (...) { auto_initialize_ = true; throw; }
    auto_initialize_ = true;
    // SetState intentionally preserves an existing drawing flag when its conditional
    // style is absent; the original GUI checkbox directly changes the flag.
    draw_text_ = state.at("_draw").get<bool>();
    if (reload) { needs_reinit_ = true; }
}
bool TextRecognition::InitializeNetwork() {
    std::lock_guard<std::recursive_mutex> lock(io_mutex_);
    try { InitDnn_(); } catch (const std::exception& error) { last_error_ = error.what(); net_load_error = true; is_initialized_ = false; net_.reset(); }
    return is_initialized_;
}
nlohmann::json TextRecognition::GetNetworkStatus() {
    std::lock_guard<std::recursive_mutex> lock(io_mutex_);
    return {{"initialized", is_initialized_}, {"needs_reinit", needs_reinit_}, {"error", last_error_}};
}
std::vector<std::pair<std::string,int>> TextRecognition::GetBackendOptions() {
    std::lock_guard<std::recursive_mutex> lock(io_mutex_);
    std::vector<std::pair<std::string,int>> result;
    for (const auto& entry: backend_list_) result.emplace_back(entry.first, static_cast<int>(entry.second));
    return result;
}
std::vector<std::pair<std::string,int>> TextRecognition::GetTargetOptions() {
    std::lock_guard<std::recursive_mutex> lock(io_mutex_);
    std::vector<std::pair<std::string,int>> result;
    for (const auto& entry: target_list_) result.emplace_back(entry.first, static_cast<int>(entry.second));
    return result;
}

}  // End Namespace DSPatch::DSPatchables
