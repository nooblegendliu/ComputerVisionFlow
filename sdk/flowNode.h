#pragma once

#include <DSPatch.h>
#include <flowProperties.h>

namespace FlowCV
{
	class Node : public DSPatch::Component
	{
	public:
		using DSPatch::Component::Component;

		FlowCV_Properties& Properties()
		{
			return properties_;
		}

		const FlowCV_Properties& Properties() const
		{
			return properties_;
		}


	protected:
		FlowCV_Properties properties_;
	};











}	// namespace FlowCV