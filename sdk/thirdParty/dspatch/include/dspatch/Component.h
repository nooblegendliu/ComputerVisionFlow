/******************************************************************************
DSPatch - The Refreshingly Simple C++ Dataflow Framework
Copyright (c) 2021, Marcus Tomlinson

BSD 2-Clause License

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
******************************************************************************/

#pragma once

#include <dspatch/SignalBus.h>
#include <dspatch/ComponentTypes.h>

#include <string>
#include <unordered_map>
#include <map>

namespace DSPatch {
	namespace internal {

		class Component;

	}	// nemaspace DSPatch::internal


	class DLLEXPORT Component {
	public:
		NONCOPYABLE(Component);
		DEFINE_PTRS(Component);

		enum class ProcessOrder {
			InOrder,
			OutOfOrder
		};

		enum class TickMode {
			Series,
			Parallel
		};

		explicit Component(ProcessOrder processOrder = ProcessOrder::InOrder);
		virtual ~Component();

		bool ConnectInput(Component::SPtr const& fromComponent, int fromOutput, int toInput);

		void DisconnectInput(int inputNo);
		void DisconnectInput(Component::SCPtr const& fromComponent);
		void DisconnectAllInputs();

		[[nodiscard]] int GetInputCount() const;
		[[nodiscard]] int GetOutputCount() const;

		[[nodiscard]] std::string GetInputName(int inputNo) const;
		[[nodiscard]] std::string GetOutputName(int outputNo) const;
		[[nodiscard]] IoType GetInputType(int inputNo) const;
		[[nodiscard]] IoType GetOutputType(int outputNo) const;

		[[nodiscard]] std::string GetComponentName() const;
		[[nodiscard]] const char* GetInstanceName() const;
		[[nodiscard]] Category GetComponentCategory() const;
		[[nodiscard]] std::string GetComponentAuthor() const;
		[[nodiscard]] std::string GetComponentVersion() const;
		void SetEnabled(bool enabled);
		bool IsEnabled();
		[[nodiscard]] int GetInstanceCount() const;
		void SetInstanceCount(int num);

		void SetBufferCount(int bufferCount);
		[[nodiscard]] int GetBufferCount() const;

		virtual bool HasGui(int interface) = 0;
		virtual void UpdateGui(void* context, int interface) = 0;
		virtual std::string GetState() = 0;
		virtual void SetState(std::string&& json_serialized) = 0;

		bool Tick(TickMode mode = TickMode::Parallel, int bufferNo = 0);
		void Reset(int bufferNo = 0);

	protected:
		virtual void Process_(SignalBus const&, SignalBus&) = 0;

		void SetInputCount_(int inputCount, std::vector<std::string> const& inputNames = {}, std::vector<IoType> const& inputTypes = {});
		void SetOutputCount_(int outputCount, std::vector<std::string> const& outputNames = {}, std::vector<IoType> const& outputTypes = {});

		void SetComponentName_(std::string component_name);
		void SetComponentCategory_(Category component_category);
		void SetComponentAuthor_(std::string component_author);
		void SetComponentVersion_(std::string component_version);

	private:
		std::unique_ptr<internal::Component> p;
		std::string name_;
		std::string instance_name_;
		int instance_count_;
		Category category_;
		std::string author_;
		std::string version_;
		bool isEnabled_;
	};






}	// namespace DSPatch