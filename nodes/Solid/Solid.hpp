#pragma once

#include <DSPatch.h>
#include <flowTypes.h>

#include <nlohmann/json.hpp>

namespace DSPatch::DSPatchables
{

	class Solid final : public Component {
	public:
		Solid();

		void UpdateGui(void* context, int interface) override;
		bool HasGui(int interface) override;

		std::string GetState() override;
		void SetState(std::string&& json_serialized) override;



	protected:
		void Process_(SignalBus const& inputs, SignalBus& outputs) override;



	};




}	// namespace DSPatchables
