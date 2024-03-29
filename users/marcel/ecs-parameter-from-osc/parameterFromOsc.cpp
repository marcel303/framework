#include "Debugging.h"
#include "osc/OscPacketListener.h"
#include "parameter.h"
#include "parameterFromOsc.h"

bool parameterFromOsc(ParameterBase * parameterBase, const osc::ReceivedMessage & m)
{
	bool result = true;

	switch (parameterBase->type)
	{
	case kParameterType_Bool:
		{
			auto * parameter = static_cast<ParameterBool*>(parameterBase);
			
			for (auto i = m.ArgumentsBegin(); i != m.ArgumentsEnd(); ++i)
			{
				if (i->IsBool())
					parameter->set(i->AsBoolUnchecked());
				else if (i->IsInt32())
					parameter->set(i->AsInt32Unchecked() != 0);
				else if (i->IsInt64())
					parameter->set(i->AsInt64Unchecked() != 0);
				else if (i->IsFloat())
					parameter->set(i->AsFloatUnchecked() != 0.f);
				else if (i->IsDouble())
					parameter->set(i->AsDoubleUnchecked() != 0.f);
				else
					result = false;
			}
		}
		break;
	case kParameterType_Int:
		{
			auto * parameter = static_cast<ParameterInt*>(parameterBase);
			
			for (auto i = m.ArgumentsBegin(); i != m.ArgumentsEnd(); ++i)
			{
				if (i->IsInt32())
					parameter->set(i->AsInt32Unchecked());
				else if (i->IsInt64())
					parameter->set(i->AsInt64Unchecked());
				else
					result = false;
			}
		}
		break;
	case kParameterType_Float:
		{
			auto * parameter = static_cast<ParameterFloat*>(parameterBase);
			
			for (auto i = m.ArgumentsBegin(); i != m.ArgumentsEnd(); ++i)
			{
				if (i->IsFloat())
					parameter->set(i->AsFloatUnchecked());
				else if (i->IsDouble())
					parameter->set(i->AsDoubleUnchecked());
				else if (i->IsInt32())
					parameter->set(i->AsInt32Unchecked() != 0);
				else if (i->IsInt64())
					parameter->set(i->AsInt64Unchecked() != 0);
				else
					result = false;
			}
		}
		break;
	case kParameterType_Vec2:
		{
			auto * parameter = static_cast<ParameterVec2*>(parameterBase);
			
			int index = 0;
			
			Vec2 value;
			
			for (auto i = m.ArgumentsBegin(); i != m.ArgumentsEnd() && index < 2; ++i)
			{
				if (i->IsFloat())
					value[index++] = i->AsFloatUnchecked();
				else if (i->IsDouble())
					value[index++] = i->AsDoubleUnchecked();
				else if (i->IsInt32())
					value[index++] = i->AsInt32Unchecked();
				else if (i->IsInt64())
					value[index++] = i->AsInt64Unchecked();
				else
					result = false;
			}
			
			if (index == 2)
				parameter->set(value);
			else
				result = false;
		}
		break;
	case kParameterType_Vec3:
		{
			auto * parameter = static_cast<ParameterVec3*>(parameterBase);
			
			int index = 0;
			
			Vec3 value;
			
			for (auto i = m.ArgumentsBegin(); i != m.ArgumentsEnd() && index < 3; ++i)
			{
				if (i->IsFloat())
					value[index++] = i->AsFloatUnchecked();
				else if (i->IsDouble())
					value[index++] = i->AsDoubleUnchecked();
				else if (i->IsInt32())
					value[index++] = i->AsInt32Unchecked();
				else if (i->IsInt64())
					value[index++] = i->AsInt64Unchecked();
				else
					result = false;
			}
			
			if (index == 3)
				parameter->set(value);
			else
				result = false;
		}
		break;
	case kParameterType_Vec4:
		{
			auto * parameter = static_cast<ParameterVec4*>(parameterBase);
			
			int index = 0;
			
			Vec4 value;
			
			for (auto i = m.ArgumentsBegin(); i != m.ArgumentsEnd() && index < 4; ++i)
			{
				if (i->IsFloat())
					value[index++] = i->AsFloatUnchecked();
				else if (i->IsDouble())
					value[index++] = i->AsDoubleUnchecked();
				else if (i->IsInt32())
					value[index++] = i->AsInt32Unchecked();
				else if (i->IsInt64())
					value[index++] = i->AsInt64Unchecked();
				else
					result = false;
			}
			
			if (index == 4)
				parameter->set(value);
			else
				result = false;
		}
		break;
	case kParameterType_String:
		{
			auto * parameter = static_cast<ParameterString*>(parameterBase);
			
			for (auto i = m.ArgumentsBegin(); i != m.ArgumentsEnd(); ++i)
			{
				if (i->IsString())
					parameter->set(i->AsStringUnchecked());
				else
					result = false;
			}
		}
		break;
	case kParameterType_Enum:
		{
			auto * parameter = static_cast<ParameterEnum*>(parameterBase);
			
			for (auto i = m.ArgumentsBegin(); i != m.ArgumentsEnd(); ++i)
			{
				if (i->IsString())
				{
					const int value = parameter->translateKeyToValue(i->AsStringUnchecked());
				
					Assert(value != -1);
					if (value != -1)
						parameter->set(value);
					else
						result = false;
				}
				else if (i->IsInt32())
				{
					const int value = i->AsInt32Unchecked();
					
					result &= parameter->set(value);
				}
				else if (i->IsInt64())
				{
					const int value = i->AsInt64Unchecked();
					
					result &= parameter->set(value);
				}
				else
					result = false;
			}
		}
		break;
		
	default:
		Assert(false);
		result = false;
		break;
	}

	return result;
}

bool parameterFromOsc(ParameterMgr & paramMgr, const osc::ReceivedMessage & m, const char * addressPattern)
{
	Assert(addressPattern[0] == '/');
	
	if (addressPattern[0] != '/')
	{
		// invalid OSC address
		
		return false;
	}
	else
	{
		const char * name = addressPattern + 1;
		const char * nameSeparator = strchr(name, '/');
		
		if (nameSeparator == nullptr || paramMgr.getStrictStructuringEnabled() == false)
		{
			// the address pattern specifies a parameter. look it up and let it handle the OSC message
			
			ParameterBase * parameter = paramMgr.find(name);
			
			if (parameter == nullptr)
			{
				if (paramMgr.getStrictStructuringEnabled())
				{
					return false;
				}
			}
			else
			{
				return parameterFromOsc(parameter, m);
			}
		}
		
		if (nameSeparator != nullptr)
		{
			// the address pattern specifies a parameter within a child. find the child and let it handle the OSC message
			
			ParameterMgr * child = nullptr;
			
			const char * nextName = nullptr;
			const char * nextNameSeparator = nullptr;
			
			bool foundIndexed = false;
			
			//
			
			const_cast<char*>(nameSeparator)[0] = 0;
			{
				// see if there's a next name and see if the next name is a number. if so, it may be an OSC address of the format "/items/0/value". where /0 specifies the index into an array
				
				nextName = nameSeparator + 1;
				nextNameSeparator = strchr(nextName, '/');
				
				bool nextNameIsNumber = false;
				int nextNumberValue = -1;
				
				if (nextNameSeparator != nullptr)
				{
					nextNameIsNumber = true;
					
					for (const char * c = nextName; c < nextNameSeparator; ++c)
						nextNameIsNumber &= c[0] >= '0' && c[0] <= '9';
					
					if (nextNameIsNumber)
					{
						const_cast<char*>(nextNameSeparator)[0] = 0;
						{
							nextNumberValue = atoi(nextName);
						}
						const_cast<char*>(nextNameSeparator)[0] = '/';
					}
				}
			
				for (auto * child_itr : paramMgr.access_children())
				{
					const std::string & child_name = child_itr->access_prefix();
					const int child_index = child_itr->access_index();
					
					if (child_name == name)
					{
						if (child_index == -1)
						{
							child = child_itr;
							break;
						}
						else if (nextNameIsNumber && child_index == nextNumberValue)
						{
							child = child_itr;
							foundIndexed = true;
							break;
						}
					}
				}
			}
			const_cast<char*>(nameSeparator)[0] = '/';
			
			if (child == nullptr)
			{
				return false;
			}
			else
			{
				return parameterFromOsc(
					*child,
					m,
					foundIndexed
						? nextNameSeparator
						: nameSeparator);
			}
		}
	}
	
	return false;
}
