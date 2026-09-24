#pragma once

#include <DSPatch.h>
#include <flowProperties.h>

namespace FlowCV
{
	class Node : public DSPatch::Component
	{
	public:
		using DSPatch::Component::Component;

		~Node() override = default;

		FlowCV_Properties& Properties()
		{
			return properties_;
		}

		const FlowCV_Properties& Properties() const
		{
			return properties_;
		}


		PropertiesSnapshot GetPropertiesSnapshot()
		{
			RefreshProperties();
			return properties_.Snapshot();
		}

		std::string GetState() override
		{
			nlohmann::json state =
				nlohmann::json::object();

			properties_.ToJson(state);

			return state.dump();
		}

		void SetState(
			std::string&& serialized) override
		{
			if (serialized.empty())
				return;

			const auto state =
				nlohmann::json::parse(serialized);

			properties_.FromJson(state);
		}

	protected:
		virtual void RefreshProperties()
		{

		}

		FlowCV_Properties properties_;
	};











}	// namespace FlowCV