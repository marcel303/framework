#pragma once

#include <string>
#include "Parameter.h"
#include "ParameterList.h"

template <class T>
class RepParam
{
public:
	inline RepParam()
	{
		m_parameter = 0;
	}

	inline Bind(Parameter* parameter)
	{
		m_parameter = parameter;
	}

	inline Bind(ParameterList& parameters, std::string name)
	{
		m_parameter = parameters[name].get();
	}

	inline T& operator=(T& value)
	{
		T* data = (T*)m_parameter->m_data;

		*data = value;

		m_parameter->Invalidate();
	}

	inline operator const T&()
	{
		T* r = (T*)m_parameter->m_data;

		return r;
	}

	Parameter* m_parameter;
};

typedef RepParam<int8> RepInt8;
typedef RepParam<int16> RepInt16;
typedef RepParam<int32> RepInt32;
typedef RepParam<float> RepFloat;
