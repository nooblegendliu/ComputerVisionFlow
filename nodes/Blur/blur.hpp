#pragma once

#include <mutex>

#include <flowNode.h>

namespace DSPatch::DSPatchables
{
	// OpenCV blur filters, port and property names remain compatible with FlowCV
	class Blur final : public FlowCV::Node
	{
	public:
		Blur();
		~Blur() override = default;

	protected:
		void Process_(SignalBus const& inputs, SignalBus& outputs) override;
		void RefreshProperties() override;

	private:
		std::mutex mutex_lock_;
	};
}	// namespace DSPatch::DSPatchables