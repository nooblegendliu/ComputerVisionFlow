#include "Viewer.hpp"

#include <exception>
#include <iostream>
#include <mutex>

#include <opencv2/core.hpp>

namespace DSPatch::DSPatchables
{

    Viewer::Viewer() 
        : FlowCV::Node(DSPatch::Component::ProcessOrder::InOrder)
    {
        // 保留原版节点信息
        SetComponentName_("Viewer");

        SetComponentCategory_(DSPatch::Category::Category_Views);

        SetComponentAuthor_("Basic");

        SetComponentVersion_("0.1.0");

        // 1 input
        SetInputCount_(1, { "in" }, { DSPatch::IoType::Io_Type_CvMat });

        // 0 outputs
        SetOutputCount_(0);

        // 保留原版默认缓存图像
        frame_ = cv::Mat(480, 640, CV_8UC3, cv::Scalar(0, 0, 0));

        SetEnabled(true);
    }


    // --------------------------------------------------------------------------
    //  Process_
    // --------------------------------------------------------------------------
    void Viewer::Process_(SignalBus const& inputs, SignalBus& outputs)
    {
        // Viewer为终端节点，没有输出端口
        (void)outputs;

        // 保留原版Viewer的行为：
        if (!IsEnabled())
            SetEnabled(true);

        // 获取输入图像
        const auto* in1 = inputs.GetValue<cv::Mat>(0);

        std::unique_lock<std::mutex> lock(io_mutex_, std::try_to_lock);

        if (!lock.owns_lock())
            return;

        // ----------------------------- No Input -------------------------------
        if (!in1)
        {
            // 保留原版 500 ms 超时行为
            const auto currentTime = std::chrono::steady_clock::now();

            const auto delta =
                std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - last_input_update_).count();

            if (delta > 500)
            {
                // 原版逻辑：
                // 已有缓存时直接清零；
                // 缓存为空时创建默认黑色图像。
                if (!frame_.empty())
                {
                    frame_.setTo(cv::Scalar(0));
                }
                else
                {
                    frame_ = cv::Mat(
                        480,
                        640,
                        CV_8UC3,
                        cv::Scalar(0, 0, 0)
                    );
                }

                return;
            }
        }
        // ------------------------------- Valid Input ----------------------------------
        else 
        {
            try {
                if (!in1->empty())
                {
                    in1->copyTo(frame_);

                    last_input_update_ = std::chrono::steady_clock::now();
                }

            }
            catch (const std::exception& e) {
                std::cout << e.what() << std::endl;
            }
        }

    }


    cv::Mat Viewer::GetFrameCopy() const
    {
        std::lock_guard<std::mutex> lock(io_mutex_);

        return frame_.clone();
    }



    // ---------------------------------------------------------------------------------
    //  GetState
    // ---------------------------------------------------------------------------------
    std::string Viewer::GetState()
    {
        return {};
    }

    // ----------------------------------------------------------------------------------
    //  SetState
    // ----------------------------------------------------------------------------------
    void Viewer::SetState(std::string&& serialized)
    {
        // 原版Viewer不恢复任何节点，保持空实现
        (void)serialized;
    }



}	// namespace DSPatch::DSPatchables