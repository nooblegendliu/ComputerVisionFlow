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

#include <dspatch/Component.h>

namespace DSPatch
{
	namespace internal
	{
		class Circuit;
	}	// namespace DSPatch::internal


	class DLLEXPORT Circuit final
	{
	public:
		NONCOPYABLE(Circuit);
		DEFINE_PTRS(Circuit);

		Circuit();
		~Circuit();

        int AddComponent(Component::SPtr const& component);

        void RemoveComponent(Component::SCPtr const& component);
        void RemoveComponent(int componentIndex);
        void RemoveAllComponents();

        [[nodiscard]] int GetComponentCount() const;

        bool ConnectOutToIn(Component::SCPtr const& fromComponent, int fromOutput, Component::SCPtr const& toComponent, int toInput);
        bool ConnectOutToIn(Component::SCPtr const& fromComponent, int fromOutput, int toComponent, int toInput);
        bool ConnectOutToIn(int fromComponent, int fromOutput, Component::SCPtr const& toComponent, int toInput);
        bool ConnectOutToIn(int fromComponent, int fromOutput, int toComponent, int toInput);

        void DisconnectComponent(Component::SCPtr const& component);
        void DisconnectComponent(int componentIndex);

        void SetBufferCount(int bufferCount);
        [[nodiscard]] int GetBufferCount() const;

        void Tick(Component::TickMode mode = Component::TickMode::Parallel);

        void StartAutoTick(Component::TickMode mode = Component::TickMode::Parallel);
        void StopAutoTick();
        void PauseAutoTick();
        void ResumeAutoTick();

    private:
        std::unique_ptr<internal::Circuit> p;
	};





		
}	// namespace DSPatch