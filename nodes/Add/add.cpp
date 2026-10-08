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
			UI Editor修改的是writeValue

			Process开始时Sync(), 
			将writeValue同步到readValue
			因此Editor和处理线程之间不直接共享裸变量
		*/
		properties_.Sync();

		auto* in1 = inputs.GetValue<cv::Mat>(0);
		if (in1 == nullptr || in1->empty()) return;

		/*
			Disable状态下延续FlowCV的Bypass行为
		*/
		if (!IsEnabled()) {
			outputs.SetValue(0, *in1);
			return;
		}
		
		auto* in2 = inputs.GetValue<cv::Mat>(1);
		if (in2 == nullptr || in2->empty()) return;

		/*
		*	Add 要求两个输入:
		*	type/channel/size 一致
		*/
		if (in1->type() != in2->type() ||
			in1->channels() != in2->channels() ||
			in1->size() != in2->size()) return;

		auto* mask = inputs.GetValue<cv::Mat>(2);
		bool hasMask = false;
		if (mask != nullptr && !mask->empty() && mask->type() == CV_8UC1 &&
			mask->size() == in1->size())
		{
			hasMask = true;
		}

		cv::Mat frame;
		if (!hasMask) cv::add(*in1, *in2, frame);
		else {
			const int maskMode = properties_.Get<int>("mask_mode");

			switch (maskMode) {
			case 1:
				// Mask 外保留Input 1
				in1->copyTo(frame);
				break;
			case 2:
				// Mask 外保留Input 2
				in2->copyTo(frame);
				break;

			case 0:
			default:
				// Mask外填充为0
				frame = cv::Mat::zeros(in1->size(), in1->type());
				break;
			}
			cv::add(*in1, *in2, frame, *mask);
			if (!frame.empty()) {
				outputs.SetValue(0, frame);
			}
		}
	}	// Add::Process_


}	// namespace DSPatch::DSPatchables
