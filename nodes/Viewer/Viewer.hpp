#pragma once

#include <chrono>
#include <mutex>
#include <string>

#include <opencv2/core/mat.hpp>

#include <flowNode.h>

namespace DSPatch::DSPatchables
{

	class Viewer final : public	FlowCV::Node
	{
	public:
		Viewer();

		~Viewer() override = default;

		// 保留原版Viewer的空状态行为
		std::string GetState() override;

		void SetState(std::string&& serialized) override;

		/*
		*	无界面Viewer的图像读取接口
		*/
		cv::Mat GetFrameCopy() const;

	protected:
		// DSPatch处理入口
		void Process_(SignalBus const& inputs, SignalBus& outputs) override;

	private:
		// 保留原版图像缓存
		cv::Mat frame_;

		// 保护图像缓存
		mutable std::mutex io_mutex_;

		// 记录上次收到有效图像的时间
		std::chrono::steady_clock::time_point last_input_update_{};

	};	// class Viewer











}	// namespace DSPatch::DSPatchables