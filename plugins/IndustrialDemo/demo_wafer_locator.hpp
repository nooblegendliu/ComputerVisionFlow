#pragma once

#include <flowNode.h>

namespace DSPatch::DSPatchables
{
	class DemoWaferLocator final : public FlowCV::Node
	{
	public:
		DemoWaferLocator();

	protected:
		void Process_(SignalBus const& inputs, SignalBus& outputs) override;
	};
}