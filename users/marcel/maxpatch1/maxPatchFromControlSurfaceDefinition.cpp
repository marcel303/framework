#include "controlSurfaceDefinition.h"
#include "maxPatchFromControlSurfaceDefinition.h"
#include "maxPatchEditor.h"
#include "StringEx.h"

bool maxPatchFromControlSurfaceDefinition(const ControlSurfaceDefinition::Surface & surface, max::Patch & patch)
{
	int nextObjectId = 1;
	
	auto allocObjectId = [&]() -> std::string
	{
		return String::FormatC("obj-%d", nextObjectId++);
	};
	
	auto connect = [&](const std::string & src, const int srcIndex, const std::string & dst, const int dstIndex)
	{
		max::PatchLine line;
		line.patchline.source = { src, String::FormatC("%d", srcIndex) };
		line.patchline.destination = { dst, String::FormatC("%d", dstIndex) };
		
		patch.patcher.lines.push_back(line);
	};
	
	max::PatchEditor patchEditor(patch);
	
	int patching_x = 30;
	int patching_y = 20;
	
	for (auto & group : surface.groups)
	{
		for (auto & elem : group.elems)
		{
			if (elem.type == ControlSurfaceDefinition::kElementType_Label)
			{
				const std::string id = allocObjectId();
				patchEditor
					.beginBox(id.c_str(), 1, 2)
						.maxclass("comment")
						.text(elem.label.text.c_str())
						.patching_rect(patching_x, patching_y, 200, 20) // comment will auto-size so size here doesn't really matter, only the position
						.presentation(true)
						.presentation_rect(elem.x, elem.y, elem.sx, elem.sy)
						.end();
				patching_y += 60;
			}
			else if (elem.type == ControlSurfaceDefinition::kElementType_Knob)
			{
				max::UnitStyle unitStyle = max::kUnitStyle_Float;
				
				switch (elem.knob.unit)
				{
					case ControlSurfaceDefinition::kUnit_Int:
						unitStyle = max::kUnitStyle_Int;
						break;
					case ControlSurfaceDefinition::kUnit_Float:
						unitStyle = max::kUnitStyle_Float;
						break;
					case ControlSurfaceDefinition::kUnit_Time:
						unitStyle = max::kUnitStyle_Time;
						break;
					case ControlSurfaceDefinition::kUnit_Hertz:
						unitStyle = max::kUnitStyle_Hertz;
						break;
					case ControlSurfaceDefinition::kUnit_Decibel:
						unitStyle = max::kUnitStyle_Decibel;
						break;
					case ControlSurfaceDefinition::kUnit_Percentage:
						unitStyle = max::kUnitStyle_Percentage;
						break;
				}
				
				std::string osc_id;
				
				if (elem.knob.oscAddress.empty() == false)
				{
					osc_id = allocObjectId();
					patchEditor
						.beginBox(osc_id.c_str(), 1, 1)
							.patching_rect(patching_x, patching_y, 200, 22) // automatic height will be 22 for 4d.paramOsc
							.text(String::FormatC("4d.paramOsc %s", elem.knob.oscAddress.c_str()).c_str())
							.end();
					patching_y += 40;
				}
				
				const std::string knob_id = allocObjectId();
				patchEditor
					.beginBox(knob_id.c_str(), 1, 2)
						.maxclass("live.dial")
						.patching_rect(patching_x, patching_y, 40, 48) // live.dial has a fixed height of 48
						.presentation(true)
						.presentation_rect(elem.x, elem.y, elem.sx, elem.sy)
						.parameter_enable(true)
						.saved_attribute("parameter_mmin", elem.knob.min)
						.saved_attribute("parameter_mmax", elem.knob.max)
						.saved_attribute("parameter_initial_enable", 1)
						.saved_attribute("parameter_initial", elem.knob.defaultValue)
						.saved_attribute("parameter_exponent", elem.knob.exponential)
						.saved_attribute("parameter_longname", elem.knob.name)
						.saved_attribute("parameter_shortname",  elem.knob.displayName)
						.saved_attribute("parameter_type", max::kParameterType_Float)
						.saved_attribute("parameter_unitstyle", unitStyle)
						.saved_attribute("parameter_linknames", 1)
						.varname(elem.knob.name.c_str())
						.end();
				patching_y += 60;
				
				if (elem.knob.oscAddress.empty() == false)
				{
					connect(osc_id, 0, knob_id, 0);
					connect(knob_id, 0, osc_id, 0);
				}
			}
			else if (elem.type == ControlSurfaceDefinition::kElementType_Listbox)
			{
				auto & listbox = elem.listbox;
				
				int defaultIndex = 0;
				for (int i = 0; i < listbox.items.size(); ++i)
					if (listbox.items[i] == listbox.defaultValue)
						defaultIndex = i;
				
				std::string osc_id;
				
				if (elem.listbox.oscAddress.empty() == false)
				{
					osc_id = allocObjectId();
					patchEditor
						.beginBox(osc_id.c_str(), 1, 1)
							.patching_rect(patching_x, patching_y, 200, 22) // automatic height will be 22 for 4d.paramOsc
							.text(String::FormatC("4d.paramOsc %s", listbox.oscAddress.c_str()).c_str())
							.end();
					patching_y += 40;
				}
				
				const std::string listbox_id = allocObjectId();
				patchEditor
					.beginBox(listbox_id.c_str(), 1, 3)
						.maxclass("live.menu")
						.patching_rect(patching_x, patching_y, 40, 48) // live.dial has a fixed height of 48
						.presentation(true)
						.presentation_rect(elem.x, elem.y, elem.sx, elem.sy)
						.parameter_enable(true)
						.saved_attribute("parameter_mmin", 0)
						.saved_attribute("parameter_mmax", (int)listbox.items.size() - 1)
						.saved_attribute("parameter_initial_enable", 1)
						.saved_attribute("parameter_initial", defaultIndex)
						.saved_attribute("parameter_longname", listbox.name)
						.saved_attribute("parameter_shortname", listbox.name)
						.saved_attribute("parameter_enum", listbox.items)
						.saved_attribute("parameter_type", max::kParameterType_Enum)
						.saved_attribute("parameter_linknames", 1)
						.varname(listbox.name.c_str())
						.end();
				patching_y += 60;
				
				if (elem.listbox.oscAddress.empty() == false)
				{
					connect(osc_id, 0, listbox_id, 0);
					connect(listbox_id, 0, osc_id, 0);
				}
			}
			else if (elem.type == ControlSurfaceDefinition::kElementType_Separator)
			{
				auto & separator = elem.separator;
				
				auto & box = patchEditor
					.beginBox(allocObjectId().c_str(), 1, 0)
						.maxclass("live.line")
						.patching_rect(patching_x, patching_y, 40, 40)
						.presentation(true)
						.presentation_rect(elem.x, elem.y, elem.sx, elem.sy);
				
				if (separator.hasBorderColor)
				{
					box.linecolor(
						separator.borderColor.r,
						separator.borderColor.g,
						separator.borderColor.b,
						separator.borderColor.a);
				}
				
				box.end();
				
				patching_y += 60;
			}
		}
		
		patching_x += 300;
		patching_y = 10;
	}

	return true;
}
