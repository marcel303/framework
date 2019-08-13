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

#define ENABLE_BACKWARD_COMPATIBLE_ROOT_PREFIXES 1 // when set to 1, parameter names pasted from clipboard don't have to start with '/' explicitly. instead names are fixes up on the spot

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
				auto value = parameter.get();

				if (ImGui::SliderInt(parameter.name.c_str(), &value, parameter.min, parameter.max))
					parameter.set(value);
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
				auto value = parameter.get();

				if (ImGui::SliderFloat(parameter.name.c_str(), &value, parameter.min, parameter.max, "%.3f", parameter.editingCurveExponential))
					parameter.set(value);
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
				auto value = parameter.get();

				if (ImGui::SliderFloat2(parameter.name.c_str(), &value[0], parameter.min[0], parameter.max[0], "%.3f", parameter.editingCurveExponential))
					parameter.set(value);
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
				auto value = parameter.get();

				if (ImGui::SliderFloat3(parameter.name.c_str(), &value[0], parameter.min[0], parameter.max[0], "%.3f", parameter.editingCurveExponential))
					parameter.set(value);
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
				auto value = parameter.get();

				if (ImGui::SliderFloat4(parameter.name.c_str(), &value[0], parameter.min[0], parameter.max[0], "%.3f", parameter.editingCurveExponential))
					parameter.set(value);
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

static void copyParameterToStringStream(ParameterBase * const parameterBase, const char * name, std::ostringstream & text)
{
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
			const int value = parameter->get();
			const char * key = parameter->translateValueToKey(value);
			text << '\t' << key;
		}
		break;
		
	default:
		Assert(false);
		break;
	}

	text << "\n";
}

static void collectParamMgrsAndPrefixes(
	ParameterMgr * const rootParamMgr,
	std::vector<ParameterMgr*> & paramMgrs,
	std::vector<std::string> & prefixes)
{
	std::vector<ParameterMgr*> stack;
	std::vector<std::string> prefixStack;
	stack.push_back(rootParamMgr);
	prefixStack.push_back(stack.back()->access_prefix() + "/");

	while (!stack.empty())
	{
		ParameterMgr * paramMgr = stack.back();
		const std::string prefix = prefixStack.back();
		
		paramMgrs.push_back(paramMgr);
		prefixes.push_back(prefix);
		
		stack.pop_back();
		prefixStack.pop_back();
		
		stack.insert(stack.end(), paramMgr->access_children().begin(), paramMgr->access_children().end());
		
		for (auto * child : paramMgr->access_children())
		{
			if (child->access_index() != -1)
			{
				const std::string child_prefix = String::FormatC("%s%s/%d/",
					prefix.c_str(),
					child->access_prefix().c_str(),
					child->access_index());
				
				prefixStack.push_back(child_prefix);
			}
			else
			{
				prefixStack.push_back(prefix + child->access_prefix() + "/");
			}
		}
	}
}

void copyParametersToText_recursive(ParameterMgr * const parameterMgr, const char * filter, std::string & out_text)
{
	std::vector<ParameterMgr*> paramMgrs;
	std::vector<std::string> prefixes;
	collectParamMgrsAndPrefixes(parameterMgr, paramMgrs, prefixes);
	
	std::ostringstream text;
	
	const bool do_filter = filter != nullptr && filter[0] != 0;
	
	for (int i = 0; i < paramMgrs.size(); ++i)
	{
		auto * paramMgr = paramMgrs[i];
		auto & prefix = prefixes[i];
		
		for (auto * parameter : paramMgr->access_parameters())
		{
			if (parameter->isSetToDefault())
				continue;
			
			const std::string name = prefix + parameter->name;
		
			if (do_filter)
			{
				if (strcasestr(name.c_str(), filter) == nullptr)
					continue;
			}
	
			copyParameterToStringStream(parameter, name.c_str(), text);
		}
	}
	
	out_text = text.str();
}

void copyParametersToClipboard_recursive(ParameterMgr * const parameterMgr, const char * filter)
{
	std::string text;
	copyParametersToText_recursive(parameterMgr, filter, text);
	
	ImGui::SetClipboardText(text.c_str());
}

static void pasteParameterFromText(ParameterBase * const parameterBase, const char * line)
{
	while (*line == '\t' || *line == ' ')
		line++;
	
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
			
			const int value = parameter->translateKeyToValue(line);
			if (value != -1)
				parameter->set(value);
		}
		break;
		
	default:
		Assert(false);
		break;
	}
}

void pasteParametersFromText_recursive(ParameterMgr * const parameterMgr, const char * text, const char * filter)
{
	// decode text
	
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

	// iterate over key-value pairs, find parameters by key, and paste value
	
	const bool do_filter = filter != nullptr && filter[0] != 0;
	
	for (auto & elem_itr : elems)
	{
		auto & elem = elem_itr.second;
		auto & name = elem_itr.first;
		auto * line = elem.line.c_str();
		
		if (name.empty())
			continue;
		
		if (do_filter && strcasestr(name.c_str(), filter) == nullptr)
			continue;
			
	#if ENABLE_BACKWARD_COMPATIBLE_ROOT_PREFIXES
		auto * parameter =
			name[0] == '/'
			? parameterMgr->findRecursively(name.c_str(), '/')
			: parameterMgr->findRecursively(("/" + name).c_str(), '/'); // less efficient backwards compatibility mode
	#else
		auto * parameter = parameterMgr->findRecursively(name.c_str(), '/');
	#endif
	
		if (parameter != nullptr)
		{
			pasteParameterFromText(parameter, line);
		}
	}
}

void pasteParametersFromClipboard_recursive(ParameterMgr * const parameterMgr, const char * filter)
{
	const char * text = ImGui::GetClipboardText();
	
 	pasteParametersFromText_recursive(parameterMgr, text, filter);
}
