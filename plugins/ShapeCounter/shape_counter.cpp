// Adapted from FlowCV Plugins/ShapeCounter (GPL-3.0).
#include "shape_counter.hpp"
#include <opencv2/imgproc.hpp>
#include <unordered_map>
#include <cmath>
#include <climits>
#include <stdexcept>
#include <iostream>
namespace DSPatch::DSPatchables {
ShapeCounter::ShapeCounter() : FlowCV::Node(ProcessOrder::OutOfOrder), state_(std::make_shared<internal::ShapeCounterState>())
{
    SetComponentName_("Shape_Counter");
    SetComponentCategory_(Category::Category_Feature_Detection);
    SetComponentAuthor_("Richard"); SetComponentVersion_("0.1.0");
    SetInputCount_(2, {"in", "shapes"}, {IoType::Io_Type_CvMat, IoType::Io_Type_JSON});
    SetOutputCount_(2, {"out", "counts"}, {IoType::Io_Type_CvMat, IoType::Io_Type_JSON});
    SetEnabled(true);
}
void ShapeCounter::Process_(SignalBus const& inputs, SignalBus& outputs)
{
    auto params = state_->Snapshot();
    const auto initialPosition = params.position;
    try {
    const auto coordinate = [](int64_t value) {
        if (value < INT_MIN || value > INT_MAX) throw std::out_of_range("Overlay coordinate overflow");
        return static_cast<int>(value);
    };
    // Input 1 Handler
    auto in1 = inputs.GetValue<cv::Mat>(0);
    auto in2 = inputs.GetValue<nlohmann::json>(1);
    if (!in1 || !in2) {
        return;
    }

    if (!in1->empty()) {
        if (IsEnabled()) {
            cv::Mat frame;
            nlohmann::json json_data;

            json_data = *in2;
            // Process Image
            in1->copyTo(frame);

            nlohmann::json json_out;
            nlohmann::json data;
            nlohmann::json entry;
            int sizeCnt = 0;
            std::unordered_map<std::string, int> classCount;
            if (params.count_mode == 0) {
                entry["size"] = params.size_match;
                entry["count"] = 0;
            }
            if (!json_data.empty()) {
                if (json_data.contains("data")) {
                    for (auto &d : json_data["data"]) {
                        if (params.count_mode == 0) {
                            if (d.contains("size")) {
                                const float rawSize = d["size"].get<float>();
                                if (!std::isfinite(rawSize) || static_cast<double>(rawSize) < INT_MIN || static_cast<double>(rawSize) > INT_MAX) continue;
                                int shapeSize = (int)rawSize;
                                if (shapeSize > (static_cast<int64_t>(params.size_match) - params.size_thresh) && shapeSize < (static_cast<int64_t>(params.size_match) + params.size_thresh)) {
                                    sizeCnt++;
                                    entry["count"] = sizeCnt;
                                }
                            }
                        }
                        else if (params.count_mode == 1) {
                            if (d.contains("class_id")) {
                                std::string classId = d["class_id"].get<std::string>();
                                if (classCount.find(classId) != classCount.end())
                                    classCount[classId]++;
                                else
                                    classCount[classId] = 1;
                            }
                        }
                    }

                    if (params.count_mode == 0)
                        data.emplace_back(entry);
                    else if (params.count_mode == 1) {
                        for (auto &cnt : classCount) {
                            nlohmann::json classEntry;
                            classEntry["class_id"] = cnt.first;
                            classEntry["count"] = cnt.second;
                            data.emplace_back(classEntry);
                        }
                    }

                    // Add Frame Overlays
                    if (params.show_overlay) {
                        if (params.count_mode == 0) {  // By Size
                            int baseline = 0;
                            int txtOffset = 12;
                            std::string countStr = "Count: ";
                            countStr += std::to_string(data.at(0)["count"].get<int>());
                            std::string sizeStr = "Size: ";
                            sizeStr += std::to_string(data.at(0)["size"].get<int>());

                            cv::Size txtSize = cv::getTextSize(countStr, params.font, params.scale, params.thickness, &baseline);
                            int roiHeight = ((txtSize.height + txtOffset) * 2) + 10;
                            int txtPosHeightOffset = (int)txtSize.height + 10;
                            cv::Rect roiRect = cv::Rect(coordinate(static_cast<int64_t>(params.position.x) - 10), coordinate(static_cast<int64_t>(params.position.y) - txtPosHeightOffset), txtSize.width + 20, roiHeight);
                            if (roiRect.x < 0) {
                                roiRect.x = 0;
                                params.position.x = 10;
                            }
                            if (roiRect.y < 0) {
                                roiRect.y = 0;
                                params.position.y = txtPosHeightOffset;
                            }
                            if (static_cast<int64_t>(roiRect.x) + roiRect.width > frame.cols) {
                                roiRect.x = ((frame.cols - roiRect.width) >= 0) ? (frame.cols - roiRect.width) : 0;
                                roiRect.width = (frame.cols - roiRect.x) - 1;
                                params.position.x = roiRect.x + 10;
                            }
                            if (static_cast<int64_t>(roiRect.y) + roiRect.height > frame.rows) {
                                roiRect.y = ((frame.rows - roiRect.height) >= 0) ? (frame.rows - roiRect.height) : 0;
                                roiRect.height = (frame.rows - roiRect.y) - 1;
                                params.position.y = roiRect.y + txtPosHeightOffset;
                            }
                            cv::Mat roi = frame(roiRect);
                            cv::Mat color(roi.size(), CV_8UC3, cv::Scalar(80, 80, 80));
                            double alpha = params.color[3];
                            if (!roi.empty()) cv::addWeighted(color, alpha, roi, 1.0 - alpha, 0.0, roi);
                            cv::rectangle(frame, roiRect, cv::Scalar(40, 40, 40), 1);
                            cv::putText(frame, sizeStr, cv::Point(params.position.x, params.position.y), params.font, params.scale,
                                cv::Scalar(params.color[2] * 255, params.color[1] * 255, params.color[0] * 255), params.thickness);
                            cv::putText(frame, countStr, cv::Point(params.position.x, params.position.y + txtSize.height + txtOffset), params.font, params.scale,
                                cv::Scalar(params.color[2] * 255, params.color[1] * 255, params.color[0] * 255), params.thickness);
                        }
                        else if (params.count_mode == 1) {  // By ID
                            if (!classCount.empty()) {
                                int baseline = 0;
                                int txtOffset = 12;
                                size_t longestClass = 0;
                                std::string longestClassStr;
                                for (auto &cnt : classCount) {
                                    if (cnt.first.size() > longestClass) {
                                        longestClass = cnt.first.size();
                                        longestClassStr = cnt.first;
                                    }
                                }
                                std::string placeholder = "Class: ";
                                placeholder += longestClassStr;
                                placeholder += ", Count: 10000";
                                cv::Size txtSize = cv::getTextSize(placeholder, params.font, params.scale, params.thickness, &baseline);
                                int roiHeight = ((txtSize.height + txtOffset) * (int)classCount.size()) + 15;
                                int txtPosHeightOffset = (int)txtSize.height + 10;
                                cv::Rect roiRect = cv::Rect(coordinate(static_cast<int64_t>(params.position.x) - 10), coordinate(static_cast<int64_t>(params.position.y) - txtPosHeightOffset), txtSize.width, roiHeight);
                                if (roiRect.x < 0) {
                                    roiRect.x = 0;
                                    params.position.x = 10;
                                }
                                if (roiRect.y < 0) {
                                    roiRect.y = 0;
                                    params.position.y = txtPosHeightOffset;
                                }
                                if (static_cast<int64_t>(roiRect.x) + roiRect.width > frame.cols) {
                                    roiRect.x = ((frame.cols - roiRect.width) >= 0) ? (frame.cols - roiRect.width) : 0;
                                    roiRect.width = (frame.cols - roiRect.x) - 1;
                                    params.position.x = roiRect.x + 10;
                                }
                                if (static_cast<int64_t>(roiRect.y) + roiRect.height > frame.rows) {
                                    roiRect.y = ((frame.rows - roiRect.height) >= 0) ? (frame.rows - roiRect.height) : 0;
                                    roiRect.height = (frame.rows - roiRect.y) - 1;
                                    params.position.y = roiRect.y + txtPosHeightOffset;
                                }

                                cv::Mat roi = frame(roiRect);
                                cv::Mat color(roi.size(), CV_8UC3, cv::Scalar(80, 80, 80));
                                double alpha = params.color[3];
                                if (!roi.empty()) cv::addWeighted(color, alpha, roi, 1.0 - alpha, 0.0, roi);
                                cv::rectangle(frame, roiRect, cv::Scalar(40, 40, 40), 1);
                                int startY = params.position.y;
                                for (auto &cnt : classCount) {
                                    std::string sizeStr = "Class: ";
                                    sizeStr += cnt.first;
                                    sizeStr += ", Count: ";
                                    sizeStr += std::to_string(cnt.second);
                                    cv::putText(frame, sizeStr, cv::Point(params.position.x, startY), params.font, params.scale,
                                        cv::Scalar(params.color[2] * 255, params.color[1] * 255, params.color[0] * 255), params.thickness);
                                    startY += (txtSize.height + txtOffset);
                                }
                            }
                        }
                    }
                }
            }
            if (!frame.empty()) {
                {
                    std::lock_guard lock(state_->mutex);
                    if (state_->params.position == initialPosition) state_->params.position = params.position;
                }
                outputs.SetValue(0, frame);
                json_out["data"] = data;
                outputs.SetValue(1, json_out);
            }
        }
        else {
            outputs.SetValue(0, *in1);
        }
    }

    } catch (const std::exception&) {
        // Malformed JSON or unsupported OpenCV input must not escape AutoTick.
    }
}
std::string ShapeCounter::GetState()
{
    const auto p = state_->Snapshot();
    return nlohmann::json{{"size_match", p.size_match}, {"size_thresh", p.size_thresh},
        {"count_mode", p.count_mode}, {"show_overlay", p.show_overlay},
        {"position", {{"x", p.position.x}, {"y", p.position.y}}}, {"scale", p.scale},
        {"font", p.font}, {"thickness", p.thickness},
        {"color", {{"R", p.color[0]}, {"G", p.color[1]}, {"B", p.color[2]}, {"A", p.color[3]}}}}.dump();
}
void ShapeCounter::SetState(std::string&& serialized)
{
    const auto j = nlohmann::json::parse(serialized);
    if (!j.is_object()) throw std::invalid_argument("Shape_Counter state must be an object");
    std::lock_guard lock(state_->mutex);
    auto p = state_->params;
    const auto integer = [&](const nlohmann::json& value) {
        if (!value.is_number_integer()) throw std::invalid_argument("Expected integer");
        const double n = value.get<double>();
        if (n < INT_MIN || n > INT_MAX) throw std::invalid_argument("Integer out of range");
        return value.get<int>();
    };
    if (j.contains("size_match")) p.size_match = integer(j.at("size_match"));
    if (j.contains("size_thresh")) p.size_thresh = integer(j.at("size_thresh"));
    if (j.contains("count_mode")) p.count_mode = integer(j.at("count_mode"));
    if (j.contains("show_overlay")) p.show_overlay = j.at("show_overlay").get<bool>();
    if (j.contains("position")) {
        p.position.x = integer(j.at("position").at("x"));
        p.position.y = integer(j.at("position").at("y"));
    }
    if (j.contains("scale")) p.scale = j.at("scale").get<float>();
    if (j.contains("font")) p.font = integer(j.at("font"));
    if (j.contains("thickness")) p.thickness = integer(j.at("thickness"));
    if (j.contains("color")) {
        const char* keys[] = {"R", "G", "B", "A"};
        for (int i = 0; i < 4; ++i) p.color[i] = j.at("color").at(keys[i]).get<float>();
    }
    if (p.count_mode < 0 || p.count_mode > 1 || !std::isfinite(p.scale))
        throw std::invalid_argument("Invalid Shape_Counter settings");
    for (float c : p.color.val)
        if (!std::isfinite(c)) throw std::invalid_argument("Invalid Shape_Counter color");
    state_->params = p;
}
#ifndef SHAPE_COUNTER_WITH_QT
EXPORT_PLUGIN(ShapeCounter)
#endif
}
