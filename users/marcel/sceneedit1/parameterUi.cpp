#include "imgui.h"
#include "parameterComponent.h"
#include "parameterUi.h"
#include "StringEx.h"

#ifdef WIN32
	#include <malloc.h>
#else
	#include <alloca.h>
#endif

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

void doParameterUi(ParameterComponent & component, const char * filter)
{
	const bool do_filter = filter != nullptr && filter[0] != 0;
	
	ParameterBase ** parameters = (ParameterBase**)alloca(component.parameters.size() * sizeof(ParameterBase*));
	
	int numParameters = 0;
	
	if (do_filter)
	{
		for (auto * parameter : component.parameters)
			if (strcasestr(parameter->name.c_str(), filter))
				parameters[numParameters++] = parameter;
	}
	else
	{
		for (auto * parameter : component.parameters)
			parameters[numParameters++] = parameter;
	}
	
	if (numParameters > 0)
	{
		if (ImGui::TreeNodeEx(&component, ImGuiTreeNodeFlags_Framed, "%s", component.prefix.c_str()))
		{
			for (int i = 0; i < numParameters; ++i)
				doParameterUi(*parameters[i]);
			
			ImGui::TreePop();
		}
	}
}

void doParameterUi(ParameterComponentMgr & componentMgr, const char * filter)
{
	// todo : implement parameter UI, with optional search filter by component prefix
	
	struct Elem
	{
		ParameterComponent * comp;
		
		bool operator<(const Elem & other) const
		{
			const int prefix_cmp = strcmp(comp->prefix.c_str(), other.comp->prefix.c_str());
			if (prefix_cmp != 0)
				return prefix_cmp < 0;
			
			const int id_cmp = strcmp(comp->id, other.comp->id);
			if (id_cmp != 0)
				return id_cmp < 0;
			
			return false;
		}
	};
	
	int numComponents = 0;
	
	for (auto * comp = componentMgr.head; comp != nullptr; comp = comp->next)
	{
		if (!comp->parameters.empty())
			numComponents++;
	}
	
	Elem * elems = (Elem*)alloca(numComponents * sizeof(Elem));
	
	int numElems = 0;
	
	for (auto * comp = componentMgr.head; comp != nullptr; comp = comp->next)
		if (!comp->parameters.empty())
			elems[numElems++].comp = comp;
	
	std::sort(elems, elems + numElems);
	
	for (int i = 0; i < numElems; ++i)
	{
		doParameterUi(*elems[i].comp, filter);
	}
}
