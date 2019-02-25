#include "imgui.h"
#include "parameterComponent.h"
#include "parameterUi.h"

void doParameterUi(ParameterBase & parameterBase)
{
	switch (parameterBase.type)
	{
	case kParameterType_Bool:
		{
			auto & parameter = static_cast<ParameterBool&>(parameterBase);
			
			ImGui::Checkbox(parameter.name.c_str(), &parameter.value);
		}
		break;
	case kParameterType_Int:
		{
			auto & parameter = static_cast<ParameterInt&>(parameterBase);
			
			if (parameter.hasLimits)
			{
				ImGui::SliderInt(parameter.name.c_str(), &parameter.value, parameter.min, parameter.max);
			}
			else
			{
				ImGui::InputInt(parameter.name.c_str(), &parameter.value);
			}
		}
		break;
	case kParameterType_Float:
		{
			auto & parameter = static_cast<ParameterFloat&>(parameterBase);
			
			if (parameter.hasLimits)
			{
				ImGui::SliderFloat(parameter.name.c_str(), &parameter.value, parameter.min, parameter.max, "%.3f", parameter.editingCurveExponential);
			}
			else
			{
				ImGui::InputFloat(parameter.name.c_str(), &parameter.value);
			}
		}
		break;
	case kParameterType_Vec2:
		{
			auto & parameter = static_cast<ParameterVec2&>(parameterBase);
			
			ImGui::InputFloat2(parameter.name.c_str(), &parameter.value[0]);
		}
		break;
	case kParameterType_Vec3:
		{
			auto & parameter = static_cast<ParameterVec3&>(parameterBase);
			
			ImGui::InputFloat3(parameter.name.c_str(), &parameter.value[0]);
		}
		break;
	case kParameterType_Vec4:
		{
			auto & parameter = static_cast<ParameterVec4&>(parameterBase);
			
			ImGui::InputFloat4(parameter.name.c_str(), &parameter.value[0]);
		}
		break;
	case kParameterType_String:
		{
			auto & parameter = static_cast<ParameterString&>(parameterBase);
			
			Assert(false); // todo : implement kParameterType_String
		}
		break;
	}
}

void doParameterUi(ParameterComponent & component)
{
	for (auto * parameter : component.parameters)
	{
		doParameterUi(*parameter);
	}
}

void doParameterUi(ParameterComponent * components, const int numComponents)
{

}
