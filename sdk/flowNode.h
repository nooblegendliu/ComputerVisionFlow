#pragma once

#include <string>

#include <DSPatch.h>
#include <flowProperties.h>

#include <nlohmann/json.hpp>

namespace FlowCV
{
	class Node : public DSPatch::Component
	{

	public:

		using DSPatch::Component::Component;

		explicit Node(
			DSPatch::Component::ProcessOrder processOrder =
			DSPatch::Component::ProcessOrder::InOrder)
			: DSPatch::Component(processOrder)
		{
			SetInstanceCount(0);
		}

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

		void SetState(std::string&& serialized) override
		{
			if (serialized.empty())
				return;

			const auto state = nlohmann::json::parse(serialized);

			// 状态恢复后重新计算依赖关系
			// 例如：属性可见性、范围等
			properties_.FromJson(state);

			// 刷新依赖属性的描述、范围和可见性
			RefreshProperties();
		}

	protected:
		virtual void RefreshProperties()
		{

		}

		FlowCV_Properties properties_;
	};











}	// namespace FlowCV