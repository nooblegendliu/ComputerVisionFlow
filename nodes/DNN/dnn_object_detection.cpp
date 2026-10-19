//
// Plugin DNN Object Detection
//

#include "dnn_object_detection.hpp"
#include <fstream>
#include <sstream>

using namespace DSPatch;
using namespace DSPatchables;



namespace DSPatch::DSPatchables
{

ObjectDetection::ObjectDetection() : FlowCV::Node(ProcessOrder::OutOfOrder)
{
    // Name and Category
    SetComponentName_("Object_Detection");
    SetComponentCategory_(DSPatch::Category::Category_DNN);
    SetComponentAuthor_("Richard");
    SetComponentVersion_("0.1.0");

    // 1 input
    SetInputCount_(1, {"in1"}, {IoType::Io_Type_CvMat});

    // 1 output
    SetOutputCount_(2, {"out", "json"}, {IoType::Io_Type_CvMat, IoType::Io_Type_JSON});

    // Set Defaults
    text_pos_ = cv::Point2i(10, 40);
    text_scale_ = 0.5;
    text_color_ = DnnColor(0.0f, 1.0f, 0.0f, 1.0f);
    text_thickness_ = 2.0;
    scale_ = 1.0f;
    conf_thresh_ = 0.5f;
    nms_thresh_ = 0.4f;
    model_res_[0] = 416;
    model_res_[1] = 416;
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

void ObjectDetection::InitDnn_()
{
    is_initialized_ = false;
    net_load_error = true;
    net_.reset();
    last_error_ = "Network initialization failed; check model and auxiliary paths.";
    // Load Classes list
    class_list_.clear();
    if (!classes_path_.empty()) {
        std::ifstream ifs(classes_path_);
        if (!ifs.is_open()) {
            std::cerr << GetInstanceName() << ", File " + classes_path_ + " not found" << std::endl;
        }
        else {
            std::string line;
            while (std::getline(ifs, line)) {
                class_list_.emplace_back(line);
            }
        }
    }

    try {
        net_ = std::make_unique<cv::dnn::Net>(cv::dnn::readNet(model_path_, config_path_, ""));
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

void ObjectDetection::DrawPredictions_(int classId, float conf, cv::Rect box, cv::Mat &frame)
{
    std::string label;
    if (!class_list_.empty()) {
        if (classId >= 0 && classId < (int)class_list_.size())
            label = class_list_.at(classId) + ": " + cv::format("%.2f", conf);
        else
            return;
    }
    else {
        label = cv::format("%.2f", conf);
    }

    rectangle(frame, cv::Point(box.x, box.y), cv::Point(box.x + box.width, box.y + box.height),
        cv::Scalar(text_color_.z * 255, text_color_.y * 255, text_color_.x * 255), text_thickness_);

    int baseLine;
    cv::Size labelSize = getTextSize(label, cv::FONT_HERSHEY_SIMPLEX, text_scale_, text_thickness_, &baseLine);

    box.y = cv::max(box.y, labelSize.height);
    rectangle(frame, cv::Point(box.x + text_pos_.x, (box.y - labelSize.height) + text_pos_.y),
        cv::Point((box.x + labelSize.width) + text_pos_.x, (box.y + baseLine) + text_pos_.y), cv::Scalar::all(64), cv::FILLED);
    putText(frame, label, cv::Point(box.x + text_pos_.x, box.y + text_pos_.y), cv::FONT_HERSHEY_SIMPLEX, text_scale_,
        cv::Scalar(text_color_.z * 255, text_color_.y * 255, text_color_.x * 255), text_thickness_);
}

void ObjectDetection::Process_(SignalBus const &inputs, SignalBus &outputs)
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
            std::vector<cv::Mat> outs;

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
                cv::dnn::blobFromImage(frame, blob, 1.0, modelSize, cv::Scalar(), swap_rb_, crop_, CV_8U);
                net_->setInput(blob, "", scale_, cv::Scalar(mean_[0], mean_[1], mean_[2]));
                if (net_->getLayer(0)->outputNameToIndex("im_info") != -1) {  // Faster-RCNN or R-FCN
                    cv::resize(frame, frame, modelSize);
                    cv::Mat imInfo = (cv::Mat_<float>(1, 3) << modelSize.height, modelSize.width, 1.6f);
                    net_->setInput(imInfo, "im_info");
                }

                net_->forward(outs, out_names_);
            }
            catch (std::exception &e) {
                std::cerr << GetInstanceName() << ", Error Computing DNN" << std::endl;
                last_error_ = e.what();
                std::cerr << e.what() << std::endl;
                return;
            }

            std::vector<int> outLayers = net_->getUnconnectedOutLayers();
            CV_Assert(!outLayers.empty());
            std::string outLayerType = net_->getLayer(outLayers[0])->type;
            std::vector<int> classIds;
            std::vector<float> confidences;
            std::vector<cv::Rect> boxes;

            if (outLayerType == "Region") {
                for (auto &out : outs) {
                    CV_Assert(out.dims == 2 && out.cols > 5 && out.depth() == CV_32F);
                    auto data = (float *)out.data;
                    for (int j = 0; j < out.rows; ++j, data += out.cols) {
                        cv::Mat scores = out.row(j).colRange(5, out.cols);
                        cv::Point classIdPoint;
                        double confidence;
                        minMaxLoc(scores, nullptr, &confidence, nullptr, &classIdPoint);
                        if (confidence > conf_thresh_) {
                            int centerX = (int)(data[0] * (float)orig.cols);
                            int centerY = (int)(data[1] * (float)orig.rows);
                            int width = (int)(data[2] * (float)orig.cols);
                            int height = (int)(data[3] * (float)orig.rows);
                            int left = centerX - width / 2;
                            int top = centerY - height / 2;

                            classIds.emplace_back(classIdPoint.x);
                            confidences.emplace_back((float)confidence);
                            boxes.emplace_back(left, top, width, height);
                        }
                    }
                }
            }
            else if (outLayerType == "DetectionOutput") {
                for (auto &out : outs) {
                    CV_Assert(out.total() % 7 == 0 && out.depth() == CV_32F);
                    auto *data = (float *)out.data;
                    for (size_t i = 0; i < out.total(); i += 7) {
                        float confidence = data[i + 2];
                        if (confidence > conf_thresh_) {
                            int left = (int)data[i + 3];
                            int top = (int)data[i + 4];
                            int right = (int)data[i + 5];
                            int bottom = (int)data[i + 6];
                            int width = right - left + 1;
                            int height = bottom - top + 1;
                            if (width <= 2 || height <= 2) {
                                left = (int)(data[i + 3] * (float)orig.cols);
                                top = (int)(data[i + 4] * (float)orig.rows);
                                right = (int)(data[i + 5] * (float)orig.cols);
                                bottom = (int)(data[i + 6] * (float)orig.rows);
                                width = right - left + 1;
                                height = bottom - top + 1;
                            }
                            if (data[i + 1] > 0) {
                                classIds.emplace_back((int)(data[i + 1]) - 1);  // Skip 0th background class id.
                                boxes.emplace_back(left, top, width, height);
                                confidences.emplace_back(confidence);
                            }
                        }
                    }
                }
            }
            else {
                std::cout << GetInstanceName() << ", Unknown output layer type: " << outLayerType << std::endl;
                return;
            }

            nlohmann::json json_out;
            nlohmann::json detected;
            json_out["data_type"] = "objects";
            nlohmann::json ref;
            ref["w"] = orig.cols;
            ref["h"] = orig.rows;
            json_out["ref_frame"] = ref;

            if (outLayers.size() > 1 || outLayerType == "Region") {
                std::vector<int> indices;
                cv::dnn::NMSBoxes(boxes, confidences, conf_thresh_, nms_thresh_, indices);
                for (int idx : indices) {
                    cv::Rect box = boxes[idx];
                    nlohmann::json cTmp;
                    if (!class_list_.empty())
                        if (classIds[idx] < class_list_.size())
                            cTmp["class"] = class_list_.at(classIds[idx]);
                    cTmp["conf"] = confidences[idx];
                    nlohmann::json jBbox;
                    jBbox["x"] = box.x;
                    jBbox["y"] = box.y;
                    jBbox["w"] = box.width;
                    jBbox["h"] = box.height;
                    cTmp["bbox"] = jBbox;
                    detected.emplace_back(cTmp);
                    if (draw_class_)
                        DrawPredictions_(classIds[idx], confidences[idx], box, orig);
                }
            }
            else {
                for (size_t idx = 0; idx < boxes.size(); ++idx) {
                    cv::Rect box = boxes[idx];
                    nlohmann::json cTmp;
                    if (!class_list_.empty())
                        if (classIds[idx] < class_list_.size())
                            cTmp["class"] = class_list_.at(classIds[idx]);
                    cTmp["conf"] = confidences[idx];
                    nlohmann::json jBbox;
                    jBbox["x"] = box.x;
                    jBbox["y"] = box.y;
                    jBbox["w"] = box.width;
                    jBbox["h"] = box.height;
                    cTmp["bbox"] = jBbox;
                    detected.emplace_back(cTmp);
                    if (draw_class_)
                        DrawPredictions_(classIds[idx], confidences[idx], box, orig);
                }
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

std::string ObjectDetection::GetState()
{
    std::lock_guard<std::recursive_mutex> stateLock(io_mutex_);
    using namespace nlohmann;

    json state;

    state["model_path"] = model_path_;
    state["config_path"] = config_path_;
    state["classes_path"] = classes_path_;
    state["mean"] = {mean_[0], mean_[1], mean_[2]};
    state["scale"] = scale_;
    state["crop"] = crop_;
    state["conf_thresh"] = conf_thresh_;
    state["preproc"] = pre_proc_resize_;
    json jResize;
    jResize["w"] = model_res_[0];
    jResize["h"] = model_res_[1];
    state["model_res"] = jResize;
    if (draw_class_) {
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

void ObjectDetection::SetState(std::string &&json_serialized)
{
    std::lock_guard<std::recursive_mutex> stateLock(io_mutex_);
    using namespace nlohmann;

    json state = json::parse(json_serialized);

    draw_class_ = false;

    if (state.contains("model_path")) {
        model_path_ = state["model_path"].get<std::string>();
    }
    if (state.contains("config_path")) {
        config_path_ = state["config_path"].get<std::string>();
    }
    if (state.contains("classes_path")) {
        classes_path_ = state["classes_path"].get<std::string>();
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
        draw_class_ = true;
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

    if (state.contains("conf_thresh"))
        conf_thresh_ = state["conf_thresh"].get<float>();

    if (state.contains("swap_rb"))
        swap_rb_ = state["swap_rb"].get<bool>();

    if (state.contains("crop"))
        crop_ = state["crop"].get<bool>();

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


nlohmann::json ObjectDetection::GetControlState() {
    std::lock_guard<std::recursive_mutex> lock(io_mutex_);
    auto state = nlohmann::json::parse(GetState());
    state["_draw"] = draw_class_;
    state["txt_color"] = {{"R", text_color_.x}, {"G", text_color_.y}, {"B", text_color_.z}};
    state["txt_position"] = {{"x", text_pos_.x}, {"y", text_pos_.y}};
    state["txt_scale"] = text_scale_;
    state["txt_thickness"] = text_thickness_;
    state["_nms_thresh"] = nms_thresh_;
    return state;
}
void ObjectDetection::UpdateControlState(const nlohmann::json& patch) {
    std::lock_guard<std::recursive_mutex> lock(io_mutex_);
    auto state = GetControlState();
    state.merge_patch(patch);
    // The original Preprocess Resize checkbox also clears Crop when switched off.
    if (patch.contains("preproc") && !patch.at("preproc").get<bool>()) state["crop"] = false;
    nms_thresh_ = state.at("_nms_thresh").get<float>();
    if (!state.at("_draw").get<bool>()) { state.erase("txt_color"); state.erase("txt_position"); state.erase("txt_scale"); state.erase("txt_thickness"); }
    const bool reload = patch.contains("model_path") || patch.contains("config_path") || patch.contains("classes_path")
        || patch.contains("colors_path") || patch.contains("voc_path") || patch.contains("backend")
        || patch.contains("target") || patch.contains("_processing_mode") || patch.contains("model_res");
    auto_initialize_ = false;
    try { SetState(state.dump()); } catch (...) { auto_initialize_ = true; throw; }
    auto_initialize_ = true;
    if (reload) { needs_reinit_ = true; }
}
bool ObjectDetection::InitializeNetwork() {
    std::lock_guard<std::recursive_mutex> lock(io_mutex_);
    try { InitDnn_(); } catch (const std::exception& error) { last_error_ = error.what(); net_load_error = true; is_initialized_ = false; net_.reset(); }
    return is_initialized_;
}
nlohmann::json ObjectDetection::GetNetworkStatus() {
    std::lock_guard<std::recursive_mutex> lock(io_mutex_);
    return {{"initialized", is_initialized_}, {"needs_reinit", needs_reinit_}, {"error", last_error_}};
}
std::vector<std::pair<std::string,int>> ObjectDetection::GetBackendOptions() {
    std::lock_guard<std::recursive_mutex> lock(io_mutex_);
    std::vector<std::pair<std::string,int>> result;
    for (const auto& entry: backend_list_) result.emplace_back(entry.first, static_cast<int>(entry.second));
    return result;
}
std::vector<std::pair<std::string,int>> ObjectDetection::GetTargetOptions() {
    std::lock_guard<std::recursive_mutex> lock(io_mutex_);
    std::vector<std::pair<std::string,int>> result;
    for (const auto& entry: target_list_) result.emplace_back(entry.first, static_cast<int>(entry.second));
    return result;
}

}  // End Namespace DSPatch::DSPatchables
