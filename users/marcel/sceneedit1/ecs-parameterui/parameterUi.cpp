#include "Debugging.h"
#include "imgui.h"
#include "parameter.h"
#include "parameterUi.h"
#include "StringEx.h"
#include <algorithm>
#include <map>
#include <sstream>

#ifdef WIN32
	#include <malloc.h>
#else
	#include <alloca.h>
#endif

void doParameterUi(ParameterBase & parameterBase)
{
	ImGui::PushID(&parameterBase);
	
	if (ImGui::Button("Default"))
		parameterBase.setToDefault();
	
	ImGui::SameLine();
	
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
		
	default:
		Assert(false);
		break;
	}
	
	ImGui::PopID();
}

void doParameterUi(ParameterMgr & parameterMgr, const char * filter, const bool showCollapsingHeader)
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
		bool isOpen = true;
		
		if (showCollapsingHeader)
		{
		// todo : this could be solved more nicely if the traversing version adds the headers
		//        and this function just does the parameter mgr UI
		
			isOpen = ImGui::TreeNodeEx(&parameterMgr, ImGuiTreeNodeFlags_Framed, "%s", parameterMgr.access_prefix().c_str());
		}
		else
		{
			ImGui::PushID(&parameterMgr);
		}
		
		if (isOpen)
		{
			ImGui::PushItemWidth(200.f);
			for (int i = 0; i < numParameters; ++i)
				doParameterUi(*parameters[i]);
			ImGui::PopItemWidth();
		}
		
		if (showCollapsingHeader)
		{
			if (isOpen)
				ImGui::TreePop();
		}
		else
		{
			ImGui::PopID();
		}
	}
}

static bool checkFilterPassesAtLeastOnce_recursive(const ParameterMgr & parameterMgr, const char * filter)
{
	Assert(filter != nullptr);
	
	// check the parameters first before recursing
	
	for (auto * parameter : parameterMgr.access_parameters())
		if (strcasestr(parameter->name.c_str(), filter))
			return true;
	
	// recurse
	
	for (auto * child : parameterMgr.access_children())
	{
		if (child->getIsHiddenFromUi())
			continue;
			
		if (checkFilterPassesAtLeastOnce_recursive(*child, filter))
			return true;
	}
	
	return false;
}

void doParameterUi_recursive(ParameterMgr & parameterMgr, const char * filter)
{
	const bool do_filter = filter != nullptr && filter[0] != 0;
	
	for (auto * child : parameterMgr.access_children())
	{
		if (child->getIsHiddenFromUi())
			continue;
		
		const char * child_filter = filter;
		
		if (do_filter)
		{
			// check the name of the parameter mgr. if it matches, show all of the child's parameters, as the filter has passed at the parent level
	
			if (strcasestr(child->access_prefix().c_str(), filter))
				child_filter = nullptr;
			else if (checkFilterPassesAtLeastOnce_recursive(*child, filter) == false)
				continue;
		}
		
		if (child->access_index() != -1)
		{
			if (ImGui::TreeNodeEx(child, ImGuiTreeNodeFlags_Framed, "%s [%d]", child->access_prefix().c_str(), child->access_index()))
			{
				doParameterUi_recursive(*child, child_filter);
				
				ImGui::TreePop();
			}
		}
		else
		{
			if (ImGui::TreeNodeEx(child, ImGuiTreeNodeFlags_Framed, "%s", child->access_prefix().c_str()))
			{
				doParameterUi_recursive(*child, child_filter);
				
				ImGui::TreePop();
			}
		}
	}
	
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
		ImGui::PushItemWidth(200.f);
		for (int i = 0; i < numParameters; ++i)
			doParameterUi(*parameters[i]);
		ImGui::PopItemWidth();
	}
}

//

void copyParametersToClipboard(ParameterBase * const * const parameters, const int numParameters)
{
	// create a list of all parameter values for each parameter which is no longer set to its default and copy the list to the clipboard

	std::ostringstream text;

	for (int i = 0; i < numParameters; ++i)
	{
		auto * parameterBase = parameters[i];
		const auto & name = parameterBase->name;
		
		if (parameterBase->isSetToDefault())
			continue;
		
		text << name << "\n";
		
		switch (parameterBase->type)
		{
		case kParameterType_Bool:
			{
				auto * parameter = static_cast<ParameterBool*>(parameterBase);
				text << '\t' << (parameter->get() ? 1 : 0);
			}
			break;
		case kParameterType_Int:
			{
				auto * parameter = static_cast<ParameterInt*>(parameterBase);
				text << '\t' << parameter->get();
			}
			break;
		case kParameterType_Float:
			{
				auto * parameter = static_cast<ParameterFloat*>(parameterBase);
				text << '\t' << parameter->get();
			}
			break;
		case kParameterType_Vec2:
			{
				auto * parameter = static_cast<ParameterVec2*>(parameterBase);
				text << '\t' << parameter->get()[0];
				text << '\t' << parameter->get()[1];
			}
			break;
		case kParameterType_Vec3:
			{
				auto * parameter = static_cast<ParameterVec3*>(parameterBase);
				text << '\t' << parameter->get()[0];
				text << '\t' << parameter->get()[1];
				text << '\t' << parameter->get()[2];
			}
			break;
		case kParameterType_Vec4:
			{
				auto * parameter = static_cast<ParameterVec4*>(parameterBase);
				text << '\t' << parameter->get()[0];
				text << '\t' << parameter->get()[1];
				text << '\t' << parameter->get()[2];
				text << '\t' << parameter->get()[3];
			}
			break;
		case kParameterType_String:
			{
				auto * parameter = static_cast<ParameterString*>(parameterBase);
				text << '\t' << parameter->get();
			}
			break;
		case kParameterType_Enum:
			{
				auto * parameter = static_cast<ParameterEnum*>(parameterBase);
				text << '\t' << parameter->get();
			}
			break;
			
		default:
			Assert(false);
			break;
		}
		
		text << "\n";
	}

	ImGui::SetClipboardText(text.str().c_str());
}

void copyParametersToClipboard(ParameterMgr * const * const parameterMgrs, const int numParameterMgrs, const char * filter)
{
	const bool do_filter = filter != nullptr && filter[0] != 0;

	int max_parameters = 0;
	
	for (int i = 0; i < numParameterMgrs; ++i)
		max_parameters += parameterMgrs[i]->access_parameters().size();
	
	ParameterBase ** const parameters = (ParameterBase ** const)alloca(max_parameters * sizeof(ParameterBase*));
	
	int numParameters = 0;
	
	if (do_filter)
	{
		for (int i = 0; i < numParameterMgrs; ++i)
			for (auto * parameter : parameterMgrs[i]->access_parameters())
				if (strcasestr(parameter->name.c_str(), filter))
					parameters[numParameters++] = parameter;
	}
	else
	{
		for (int i = 0; i < numParameterMgrs; ++i)
			for (auto * parameter : parameterMgrs[i]->access_parameters())
				parameters[numParameters++] = parameter;
	}
	
	copyParametersToClipboard(parameters, numParameters);
}

void pasteParametersFromClipboard(ParameterBase * const * const parameters, const int numParameters)
{
	// parse the list of parameter values from clipboard and update all of the known parameters accordingly
	
	const char * text = ImGui::GetClipboardText();
	
	struct Elem
	{
		std::string line;
	};

	std::map<std::string, Elem> elems;

	std::istringstream str(text);

	std::string line;

	int index = 0;

	Elem * elem = nullptr;

	for (;;)
	{
		std::getline(str, line);
		
		if (str.eof() || str.fail())
			break;
		
		if (index == 0)
		{
			elem = &elems[line];
		}
		else
		{
			std::swap(elem->line, line);
		}
		
		index++;
		
		if (index == 2)
			index = 0;
	}

	//
	
	for (int i = 0; i < numParameters; ++i)
	{
		auto * parameterBase = parameters[i];
		const auto & name = parameterBase->name;
		
		auto elem_itr = elems.find(name);
		
		if (elem_itr == elems.end())
			continue;
		
		auto & elem = elem_itr->second;
		auto * line = elem.line.c_str();
		
		switch (parameterBase->type)
		{
		case kParameterType_Bool:
			{
				auto * parameter = static_cast<ParameterBool*>(parameterBase);
				
				int value;
				if (sscanf(line, "%d", &value) == 1)
					parameter->set(value != 0);
			}
			break;
		case kParameterType_Int:
			{
				auto * parameter = static_cast<ParameterInt*>(parameterBase);
				
				int value;
				if (sscanf(line, "%d", &value) == 1)
					parameter->set(value);
			}
			break;
		case kParameterType_Float:
			{
				auto * parameter = static_cast<ParameterFloat*>(parameterBase);
				
				float value;
				if (sscanf(line, "%f", &value) == 1)
					parameter->set(value);
			}
			break;
		case kParameterType_Vec2:
			{
				auto * parameter = static_cast<ParameterVec2*>(parameterBase);
				
				float values[2];
				if (sscanf(line, "%f %f", &values[0], &values[1]) == 2)
					parameter->set(Vec2(values[0], values[1]));
			}
			break;
		case kParameterType_Vec3:
			{
				auto * parameter = static_cast<ParameterVec3*>(parameterBase);
				
				float values[3];
				if (sscanf(line, "%f %f %f", &values[0], &values[1], &values[2]) == 3)
					parameter->set(Vec3(values[0], values[1], values[2]));
			}
			break;
		case kParameterType_Vec4:
			{
				auto * parameter = static_cast<ParameterVec4*>(parameterBase);
				
				float values[4];
				if (sscanf(line, "%f %f %f %f", &values[0], &values[1], &values[2], &values[3]) == 4)
					parameter->set(Vec4(values[0], values[1], values[2], values[3]));
			}
			break;
		case kParameterType_String:
			{
				auto * parameter = static_cast<ParameterString*>(parameterBase);
				
				parameter->set(line);
			}
			break;
		case kParameterType_Enum:
			{
				auto * parameter = static_cast<ParameterEnum*>(parameterBase);
				
				int value;
				if (sscanf(line, "%d", &value) == 1)
					parameter->set(value);
			}
			break;
			
		default:
			Assert(false);
			break;
		}
	}
}

void pasteParametersFromClipboard(ParameterMgr * const * const parameterMgrs, const int numParameterMgrs, const char * filter)
{
	const bool do_filter = filter != nullptr && filter[0] != 0;

	int max_parameters = 0;
	
	for (int i = 0; i < numParameterMgrs; ++i)
		max_parameters += parameterMgrs[i]->access_parameters().size();
	
	ParameterBase ** const parameters = (ParameterBase ** const)alloca(max_parameters * sizeof(ParameterBase*));
	
	int numParameters = 0;
	
	if (do_filter)
	{
		for (int i = 0; i < numParameterMgrs; ++i)
			for (auto * parameter : parameterMgrs[i]->access_parameters())
				if (strcasestr(parameter->name.c_str(), filter))
					parameters[numParameters++] = parameter;
	}
	else
	{
		for (int i = 0; i < numParameterMgrs; ++i)
			for (auto * parameter : parameterMgrs[i]->access_parameters())
				parameters[numParameters++] = parameter;
	}
	
	pasteParametersFromClipboard(parameters, numParameters);
}
