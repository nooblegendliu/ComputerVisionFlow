#pragma once

#include <chrono>
#include <mutex>
#include <string>

#include <flowNode.h>

namespace DSPatch::DSPatchables
{

	class Solid final : public FlowCV::Node
	{
	public:
		Solid();

		~Solid() override = default;

		// 保留FlowCV的Json状态格式
		std::string GetState() override;

		void SetState(std::string&& serialized) override;

	protected:
		// 保留DSPatch原版入口
		void Process_(SignalBus const& inputs, SignalBus& outputs) override;

		// 根据参数更新属性可见性，不依赖UI框架
		void RefreshProperties() override;

	private:
		std::mutex io_mutex_;

		// 帧率控制所使用的时间点
		std::chrono::steady_clock::time_point last_time_;
	};	// class Solid








}	// namespace DSPatch::DSPatchables