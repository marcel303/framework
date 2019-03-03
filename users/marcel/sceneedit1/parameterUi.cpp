#include "imgui.h"
#include "parameterComponent.h"
#include "parameterUi.h"
#include "StringEx.h"

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
			
			if (parameter.hasLimits)
			{
				// fixme : no separate min/max for each dimension
				ImGui::SliderFloat2(parameter.name.c_str(), &parameter.value[0], parameter.min[0], parameter.max[0]);
			}
			else
			{
				ImGui::InputFloat2(parameter.name.c_str(), &parameter.value[0]);
			}
		}
		break;
	case kParameterType_Vec3:
		{
			auto & parameter = static_cast<ParameterVec3&>(parameterBase);
			
			if (parameter.hasLimits)
			{
				// fixme : no separate min/max for each dimension
				ImGui::SliderFloat3(parameter.name.c_str(), &parameter.value[0], parameter.min[0], parameter.max[0]);
			}
			else
			{
				ImGui::InputFloat3(parameter.name.c_str(), &parameter.value[0]);
			}
		}
		break;
	case kParameterType_Vec4:
		{
			auto & parameter = static_cast<ParameterVec4&>(parameterBase);
			
			if (parameter.hasLimits)
			{
				// fixme : no separate min/max for each dimension
				ImGui::SliderFloat4(parameter.name.c_str(), &parameter.value[0], parameter.min[0], parameter.max[0]);
			}
			else
			{
				ImGui::InputFloat4(parameter.name.c_str(), &parameter.value[0]);
			}
		}
		break;
	case kParameterType_String:
		{
			auto & parameter = static_cast<ParameterString&>(parameterBase);
			
			char buffer[1024];
			strcpy_s(buffer, sizeof(buffer), parameter.value.c_str());
			
			if (ImGui::InputText(parameter.name.c_str(), buffer, sizeof(buffer)))
			{
				parameter.value = buffer;
			}
		}
		break;
	}
}

void doParameterUi(ParameterComponent & component)
{
	if (ImGui::TreeNodeEx(&component, ImGuiTreeNodeFlags_CollapsingHeader, "%s", component.prefix.c_str()))
	{
		for (auto * parameter : component.parameters)
		{
			doParameterUi(*parameter);
		}
	}
}

void doParameterUi(ParameterComponent ** components, const int numComponents)
{
	// todo : implement parameter UI, with optional search filter by component prefix
	
	for (int i = 0; i < numComponents; ++i)
	{
		doParameterUi(*components[i]);
	}
}
