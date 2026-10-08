#ifndef VISIONFLOW_PLUGIN_ADD_HPP_
#define VISIONFLOW_PLUGIN_ADD_HPP_

#include <flowNode.h>

namespace DSPatch::DSPatchables
{
	class Add final : public FlowCV::Node
	{
	public:
		Add();

		~Add() override = default;

	protected:
		void Process_(SignalBus const& inputs, SignalBus& outputs) override;
	};
}




#endif	// VISIONFLOW_PLUGIN_ADD_HPP_