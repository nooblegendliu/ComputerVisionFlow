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

#include <dspatch/Common.h>

namespace DSPatch
{
	// Type Erasure设计模式
	// 类似 std/boost::any或者QVariant
	// 用于在数据流中传递
	class DLLEXPORT Signal final
	{
	public:
		NONCOPYABLE(Signal);
		DEFINE_PTRS(Signal);

		Signal();
		~Signal();

		[[nodiscard]] bool HasValue() const;

		template <class ValueType>
		ValueType* GetValue();

		template <class ValueType>
		void SetValue(ValueType const& newVal);

		bool CopySignal(Signal::SPtr const& fromSignal);
		bool MoveSignal(Signal::SPtr const& fromSignal);

		void ClearValue();

		[[nodiscard]] std::type_info const& GetType() const;

	private:
		struct _ValueHolder
		{
			NONCOPYABLE(_ValueHolder);

			_ValueHolder() = default;
			virtual ~_ValueHolder() = default;

			virtual std::type_info const& GetType() const = 0;
			virtual _ValueHolder* GetCopy() const = 0;
			virtual void SetValue(_ValueHolder* valueHolder) = 0;
		};

		template <class ValueType>
		struct _Value final : _ValueHolder
		{
			NONCOPYABLE(_Value);

			_Value(ValueType const& value) : value(value), type(typeid(ValueType))
			{ }

			virtual std::type_info const& GetType() const override
			{
				return type;
			}

			virtual _ValueHolder* GetCopy() const override
			{
				return new _Value(value);
			}

			virtual void SetValue(_ValueHolder* valueHolder) override
			{
				value = ((_Value<ValueType>*)valueHolder)->value;
			}

			ValueType value;
			std::type_info const& type;
		};

		_ValueHolder* _valueHolder = nullptr;
		bool _hasValue = false;
	};


	// 类中函数的实现
	template <class ValueType>
	ValueType* Signal::GetValue()
	{
		if (_hasValue && GetType() == typeid(ValueType))
		{
			return&((_Value<ValueType>*)_valueHolder)->value;
		}
		else {
			return nullptr;
		}
	}

	template <class ValueType>
	void Signal::SetValue(ValueType const& newValue)
	{
		if (GetType() == typeid(ValueType)) {
			((_Value<ValueType>*)_valueHolder)->value = newValue;
		}
		else {
			delete _valueHolder;
			_valueHolder = new _Value<ValueType>(newValue);
		}
		_hasValue = true;
	}


}	// namespace DSPatch