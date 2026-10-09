#include "add.hpp"

#include <opencv2/core.hpp>

namespace DSPatch::DSPatchables
{
	Add::Add()
		: FlowCV::Node(DSPatch::Component::ProcessOrder::OutOfOrder)
	{
		SetComponentName_("Add");

		SetComponentCategory_(DSPatch::Category::Category_Merge);

		SetComponentAuthor_("BASIC");

		SetComponentVersion_("0.1.0");

		// Input:
		// 0 -> image 1
		// 1 -> image 2
		// 2 -> optional mask
		SetInputCount_(3,
			{ "in1","in2","mask" },
			{
				DSPatch::IoType::Io_Type_CvMat,
				DSPatch::IoType::Io_Type_CvMat,
				DSPatch::IoType::Io_Type_CvMat
			}
		);

		SetOutputCount_(1,
			{ "out" },
			{ DSPatch::IoType::Io_Type_CvMat });

		// 用属性代替原理的mask_mode_成员变量
		properties_.AddOption(
			"mask_mode", "Mask Mode", 0,
			{
				"Default",
				"In1",
				"In2"
			});

		SetEnabled(true);
	}


	void Add::Process_(SignalBus const& inputs, SignalBus& outputs)
	{
		/*
		*	编辑参数同步到处理函数
		*/
		properties_.Sync();

		// 获取输入
		const auto* in1 = inputs.GetValue<cv::Mat>(0);
		const auto* in2 = inputs.GetValue<cv::Mat>(1);
		const auto* mask = inputs.GetValue<cv::Mat>(2);

		if (!in1 || !in2 || in1->empty() || in2->empty()) return;		// 保留原有双输入要求

		// 禁用节点时输出第一路图像
		if (!IsEnabled()) {
			outputs.SetValue(0, *in1);
			return;
		}

		// 检查输入图像的类型和尺寸
		if (in1->type() != in2->type() || in1->size() != in2->size()) return;

		// 判断mask是否有效
		bool hasMask = false;

		if (mask && !mask->empty() && mask->type() == CV_8UC1 && mask->size() == in1->size())
		{
			hasMask = true;
		}

		cv::Mat frame;
		if (hasMask) {
			const int maskMode = properties_.Get<int>("mask_mode");

			switch (maskMode)
			{
			case 0:
			default:
				// Mask外使用黑色
				frame = cv::Mat::zeros(in1->size(), in1->type());
				break;

			case 1:
				// Mask外保留第一路输入
				in1->copyTo(frame);
				break;

			case 2:
				// Mask外保留第二路输入
				in2->copyTo(frame);
				break;
			}
			cv::add(*in1, *in2, frame, *mask);
		}
		else {
			// 不使用mask
			cv::add(*in1, *in2, frame);
		}

		// 两条路径统一输出
		if (!frame.empty()) {
			outputs.SetValue(0, frame);
		}
	}	// Add::Process_


}	// namespace DSPatch::DSPatchables
