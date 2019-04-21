#include "imgui.h"
#include "parameter.h"
#include "parameterUi.h"
#include "StringEx.h"
#include <algorithm>

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
			
			if (ImGui::Checkbox(parameter.name.c_str(), &parameter.access_rw()))
				parameter.setDirty();
		}
		break;
	case kParameterType_Int:
		{
			auto & parameter = static_cast<ParameterInt&>(parameterBase);
			
			if (parameter.hasLimits)
			{
				if (ImGui::SliderInt(parameter.name.c_str(), &parameter.access_rw(), parameter.min, parameter.max))
					parameter.setDirty();
			}
			else
			{
				if (ImGui::InputInt(parameter.name.c_str(), &parameter.access_rw()))
					parameter.setDirty();
			}
		}
		break;
	case kParameterType_Float:
		{
			auto & parameter = static_cast<ParameterFloat&>(parameterBase);
			
			if (parameter.hasLimits)
			{
				if (ImGui::SliderFloat(parameter.name.c_str(), &parameter.access_rw(), parameter.min, parameter.max, "%.3f", parameter.editingCurveExponential))
					parameter.setDirty();
			}
			else
			{
				if (ImGui::InputFloat(parameter.name.c_str(), &parameter.access_rw()))
					parameter.setDirty();
			}
		}
		break;
	case kParameterType_Vec2:
		{
			auto & parameter = static_cast<ParameterVec2&>(parameterBase);
			
			if (parameter.hasLimits)
			{
				// fixme : no separate min/max for each dimension
				if (ImGui::SliderFloat2(parameter.name.c_str(), &parameter.access_rw()[0], parameter.min[0], parameter.max[0]))
					parameter.setDirty();
			}
			else
			{
				if (ImGui::InputFloat2(parameter.name.c_str(), &parameter.access_rw()[0]))
					parameter.setDirty();
			}
		}
		break;
	case kParameterType_Vec3:
		{
			auto & parameter = static_cast<ParameterVec3&>(parameterBase);
			
			if (parameter.hasLimits)
			{
				// fixme : no separate min/max for each dimension
				if (ImGui::SliderFloat3(parameter.name.c_str(), &parameter.access_rw()[0], parameter.min[0], parameter.max[0]))
					parameter.setDirty();
			}
			else
			{
				if (ImGui::InputFloat3(parameter.name.c_str(), &parameter.access_rw()[0]))
					parameter.setDirty();
			}
		}
		break;
	case kParameterType_Vec4:
		{
			auto & parameter = static_cast<ParameterVec4&>(parameterBase);
			
			if (parameter.hasLimits)
			{
				// fixme : no separate min/max for each dimension
				if (ImGui::SliderFloat4(parameter.name.c_str(), &parameter.access_rw()[0], parameter.min[0], parameter.max[0]))
					parameter.setDirty();
			}
			else
			{
				if (ImGui::InputFloat4(parameter.name.c_str(), &parameter.access_rw()[0]))
					parameter.setDirty();
			}
		}
		break;
	case kParameterType_String:
		{
			auto & parameter = static_cast<ParameterString&>(parameterBase);
			
			char buffer[1024];
			strcpy_s(buffer, sizeof(buffer), parameter.get().c_str());
			
			if (ImGui::InputText(parameter.name.c_str(), buffer, sizeof(buffer)))
			{
				parameter.set(buffer);
			}
		}
		break;
	case kParameterType_Enum:
		{
			auto & parameter = static_cast<ParameterEnum&>(parameterBase);
			
			auto & elems = parameter.getElems();
			
			int currentItemIndex = -1;
			
			const int numItems = elems.size();
			const char ** items = (const char **)alloca(numItems * sizeof(char*));
			
			int itemIndex = 0;
			
			for (auto & elem : elems)
			{
				items[itemIndex] = elem.key;
				
				if (elem.value == parameter.get())
					currentItemIndex = itemIndex;
				
				itemIndex++;
			}
			
			if (ImGui::Combo(parameter.name.c_str(), &currentItemIndex, items, numItems))
			{
				parameter.set(elems[currentItemIndex].value);
			}
		}
		break;
	}
}

void doParameterUi(ParameterMgr & parameterMgr, const char * filter)
{
	const bool do_filter = filter != nullptr && filter[0] != 0;
	
	ParameterBase ** parameters = (ParameterBase**)alloca(parameterMgr.access_parameters().size() * sizeof(ParameterBase*));
	
	int numParameters = 0;
	
	if (do_filter)
	{
		for (auto * parameter : parameterMgr.access_parameters())
			if (strcasestr(parameter->name.c_str(), filter))
				parameters[numParameters++] = parameter;
	}
	else
	{
		for (auto * parameter : parameterMgr.access_parameters())
			parameters[numParameters++] = parameter;
	}
	
	if (numParameters > 0)
	{
		if (ImGui::TreeNodeEx(&parameterMgr, ImGuiTreeNodeFlags_Framed, "%s", parameterMgr.access_prefix().c_str()))
		{
			for (int i = 0; i < numParameters; ++i)
				doParameterUi(*parameters[i]);
			
			ImGui::TreePop();
		}
	}
}
