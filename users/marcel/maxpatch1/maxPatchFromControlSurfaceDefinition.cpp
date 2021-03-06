#include "controlSurfaceDefinition.h"
#include "maxPatchFromControlSurfaceDefinition.h"
#include "maxPatchEditor.h"
#include "StringEx.h"
#include <algorithm>

static bool cut_string_by_substring(
	const std::string & str,
	const std::string & substring,
	std::string & before,
	std::string & after)
{
	auto pos = str.find(substring);
	
	if (pos == std::string::npos)
		return false;
	else
	{
		before = str.substr(0, pos);
		after = str.substr(pos + substring.length());
		return true;
	}
}

static std::string fixupOscAddress(const char * path)
{
	std::string result;
	
	std::string before;
	std::string after;
	
	if (cut_string_by_substring(path, "/1/", before, after))
	{
		result = "/" + after + " @prefix " + before + "/";
	}
	else
	{
		result = std::string(path) + " @indexEnable 0";
	}
	
	return result;
}

static void rgbToHsl(const float r, const float g, const float b, float & hue, float & sat, float & lum)
{
	float max = std::max(r, std::max(g, b));
	float min = std::min(r, std::min(g, b));

	lum = (max + min) / 2.0f;

	float delta = max - min;

	if (delta < 1e-10f)
	{
		sat = 0.f;
		hue = 0.f;
	}
	else
	{
		sat = (lum <= .5f) ? (delta / (max + min)) : (delta / (2.f - (max + min)));

		if (r == max)
			hue = (g - b) / delta;
		else if (g == max)
			hue = 2.f + (b - r) / delta;
		else
			hue = 4.f + (r - g) / delta;

		if (hue < 0.f)
			hue += 6.0f;

		hue /= 6.f;
	}
}

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
	
	patchEditor.rect(0, 0, surface.layout.sx, surface.layout.sy);
	
	int patching_x = 30;    
	int patching_y = 20;
	
	if (surface.name.empty() == false)
	{
		const std::string autopattr_name = String::Replace(surface.name, '/', '_');
		
		patchEditor.beginBox(allocObjectId().c_str(), 1, 4)
			.maxclass("newobj")
			.text(String::FormatC("autopattr %s", autopattr_name.c_str()).c_str())
			.patching_rect(patching_x, patching_y, 450, 20)
			.varname(autopattr_name.c_str())
			.end();
		patching_y += 60;
	}

	for (auto & group : surface.groups)
	{
		for (auto & elem : group.elems)
		{
			auto * elemLayout = surface.layout.findElement(group.name.c_str(), elem.name.c_str());
			
			if (elem.type == ControlSurfaceDefinition::kElementType_Label)
			{
				const std::string id = allocObjectId();
				patchEditor
					.beginBox(id.c_str(), 1, 2)
						.maxclass("comment")
						.text(elem.label.text.c_str())
						.patching_rect(patching_x, patching_y, 200, 20) // comment will auto-size so size here doesn't really matter, only the position
						.presentation(true)
						.presentation_rect(elemLayout->x, elemLayout->y, elemLayout->sx, elemLayout->sy)
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
							.patching_rect(patching_x, patching_y, 450, 22) // automatic height will be 22 for 'paramOsc' box
							.text(String::FormatC("paramOsc %s", fixupOscAddress(elem.knob.oscAddress.c_str()).c_str()).c_str())
							.end();
					patching_y += 40;
				}
				
				const std::string knob_id = allocObjectId();
				patchEditor
					.beginBox(knob_id.c_str(), 1, 2)
						.maxclass("live.dial")
						.patching_rect(patching_x, patching_y, 40, 48) // live.dial has a fixed height of 48
						.presentation(true)
						.presentation_rect(elemLayout->x, elemLayout->y, elemLayout->sx, elemLayout->sy)
						.parameter_enable(true)
						.saved_attribute("parameter_mmin", elem.knob.min)
						.saved_attribute("parameter_mmax", elem.knob.max)
						.saved_attribute("parameter_initial_enable", elem.knob.hasDefaultValue)
						.saved_attribute("parameter_initial", elem.knob.defaultValue)
						.saved_attribute("parameter_exponent", elem.knob.exponential)
						.saved_attribute("parameter_longname", elem.name)
						.saved_attribute("parameter_shortname",  elem.knob.displayName)
						.saved_attribute("parameter_type", max::kParameterType_Float)
						.saved_attribute("parameter_unitstyle", unitStyle)
						.saved_attribute("parameter_linknames", 1)
						.varname(elem.name.c_str())
						.end();
				patching_y += 60;
				
				if (elem.knob.oscAddress.empty() == false)
				{
					connect(osc_id, 0, knob_id, 0);
					connect(knob_id, 0, osc_id, 0);
				}
			}
			else if (elem.type == ControlSurfaceDefinition::kElementType_Button)
			{
				// todo : implement button output
				// todo : add support for trigger type buttons and toggle buttons
				std::string osc_id;
				
				if (elem.button.oscAddress.empty() == false)
				{
					osc_id = allocObjectId();
					patchEditor
						.beginBox(osc_id.c_str(), 1, 1)
							.patching_rect(patching_x, patching_y, 450, 22) // automatic height will be 22 for 'paramOsc' box
							.text(String::FormatC("paramOsc %s", fixupOscAddress(elem.button.oscAddress.c_str()).c_str()).c_str())
							.end();
					patching_y += 40;
				}
				
				const std::string button_id = allocObjectId();
				patchEditor
					.beginBox(button_id.c_str(), 1, 2)
						.maxclass("live.text")
						.text(elem.name.c_str())
						.mode(0)
						.patching_rect(patching_x, patching_y, 40, 48) // live.dial has a fixed height of 48
						.presentation(true)
						.presentation_rect(elemLayout->x, elemLayout->y, elemLayout->sx, elemLayout->sy)
						.parameter_enable(true)
						.saved_attribute("parameter_enum", std::vector<int>({ 1, 1 }))
						.saved_attribute("parameter_type", max::kParameterType_Enum)
						.saved_attribute("parameter_mmax", 1)
						.saved_attribute("parameter_longname", elem.name)
						.saved_attribute("parameter_shortname",  elem.button.displayName)
						.saved_attribute("parameter_linknames", 1)
						.varname(elem.name.c_str())
						.end();
				patching_y += 60;
				
				if (elem.button.oscAddress.empty() == false)
				{
					connect(osc_id, 0, button_id, 0);
					connect(button_id, 0, osc_id, 0);
				}
			}
			else if (elem.type == ControlSurfaceDefinition::kElementType_Slider2)
			{
				const int label_y = elemLayout->y;
				const int label_sy = 20;
				const int numbox_y = elemLayout->y + 20;
				const int numbox_sy = elemLayout->sy - 20;
				
				patchEditor
					.beginBox(allocObjectId().c_str(), 1, 2)
						.maxclass("comment")
						.text(elem.name.c_str())
						.patching_rect(patching_x, patching_y, 200, 20) // comment will auto-size so size here doesn't really matter, only the position
						.presentation(true)
						.presentation_rect(elemLayout->x, label_y, elemLayout->sx, label_sy)
						.end();
				patching_y += 60;
				
				std::string elem_ids[2];
				
				for (int i = 0; i < 2; ++i)
				{
					const char * elem_postfix = "xy";
					
					const std::string elem_id = allocObjectId();
					const std::string elem_name = elem.name + "." + elem_postfix[i];
					
					elem_ids[i] = elem_id;
					
					patchEditor
						.beginBox(elem_id.c_str(), 1, 2)
							.maxclass("live.numbox")
							.patching_rect(patching_x, patching_y, 40, 15) // live.numbox has a fixed height of 15
							.presentation(true)
							.presentation_rect(elemLayout->x + elemLayout->sx / 2 * i, numbox_y, elemLayout->sx / 2, numbox_sy)
							.parameter_enable(true)
							.saved_attribute("parameter_mmin", elem.slider2.min[i])
							.saved_attribute("parameter_mmax", elem.slider2.max[i])
							.saved_attribute("parameter_initial_enable", elem.slider2.hasDefaultValue)
							.saved_attribute("parameter_initial", elem.slider2.defaultValue[i])
							.saved_attribute("parameter_longname", elem_name)
							.saved_attribute("parameter_shortname",  elem.slider2.displayName)
							.saved_attribute("parameter_type", max::kParameterType_Float)
							.saved_attribute("parameter_unitstyle", max::kUnitStyle_Float)
							.saved_attribute("parameter_linknames", 1)
							.varname(elem_name.c_str())
							.end();
					patching_y += 20;
				}
				
				if (elem.slider2.oscAddress.empty() == false)
				{
					const std::string pak_id = allocObjectId();
					
					patchEditor
						.beginBox(pak_id.c_str(), 4, 1)
							.maxclass("newobj")
							.patching_rect(patching_x, patching_y, 200, 22) // newobj has a fixed height of 22
							.parameter_enable(true)
							.text("pak f f f")
							.end();
					patching_y += 30;

					const std::string osc_id = allocObjectId();
					patchEditor
						.beginBox(osc_id.c_str(), 1, 1)
							.patching_rect(patching_x, patching_y, 450, 22) // automatic height will be 22 for 'paramOsc' box
							.text(String::FormatC("paramOsc %s", fixupOscAddress(elem.slider2.oscAddress.c_str()).c_str()).c_str())
							.end();
					patching_y += 40;
					
					connect(elem_ids[0], 0, pak_id, 0);
					connect(elem_ids[1], 0, pak_id, 1);
					
					connect(pak_id, 0, osc_id, 0);
					
					connect(osc_id, 0, elem_ids[0], 0);
					connect(osc_id, 0, elem_ids[1], 0);
				}
			}
			else if (elem.type == ControlSurfaceDefinition::kElementType_Slider3)
			{
				const int label_y = elemLayout->y;
				const int label_sy = 20;
				const int numbox_y = elemLayout->y + 20;
				const int numbox_sy = elemLayout->sy - 20;
				
				patchEditor
					.beginBox(allocObjectId().c_str(), 1, 2)
						.maxclass("comment")
						.text(elem.name.c_str())
						.patching_rect(patching_x, patching_y, 200, 20) // comment will auto-size so size here doesn't really matter, only the position
						.presentation(true)
						.presentation_rect(elemLayout->x, label_y, elemLayout->sx, label_sy)
						.end();
				patching_y += 60;
				
				std::string elem_ids[3];
				
				for (int i = 0; i < 3; ++i)
				{
					const char * elem_postfix = "xyz";
					
					const std::string elem_id = allocObjectId();
					const std::string elem_name = elem.name + "." + elem_postfix[i];
					
					elem_ids[i] = elem_id;
					
					patchEditor
						.beginBox(elem_id.c_str(), 1, 2)
							.maxclass("live.numbox")
							.patching_rect(patching_x, patching_y, 40, 15) // live.numbox has a fixed height of 15
							.presentation(true)
							.presentation_rect(elemLayout->x + elemLayout->sx / 3 * i, numbox_y, elemLayout->sx / 3, numbox_sy)
							.parameter_enable(true)
							.saved_attribute("parameter_mmin", elem.slider3.min[i])
							.saved_attribute("parameter_mmax", elem.slider3.max[i])
							.saved_attribute("parameter_initial_enable", elem.slider3.hasDefaultValue)
							.saved_attribute("parameter_initial", elem.slider3.defaultValue[i])
							.saved_attribute("parameter_longname", elem_name)
							.saved_attribute("parameter_shortname",  elem.slider3.displayName)
							.saved_attribute("parameter_type", max::kParameterType_Float)
							.saved_attribute("parameter_unitstyle", max::kUnitStyle_Float)
							.saved_attribute("parameter_linknames", 1)
							.varname(elem_name.c_str())
							.end();
					patching_y += 20;
				}
				
				if (elem.slider3.oscAddress.empty() == false)
				{
					const std::string pak_id = allocObjectId();
					
					patchEditor
						.beginBox(pak_id.c_str(), 4, 1)
							.maxclass("newobj")
							.patching_rect(patching_x, patching_y, 200, 22) // newobj has a fixed height of 22
							.parameter_enable(true)
							.text("pak f f f")
							.end();
					patching_y += 30;

					const std::string osc_id = allocObjectId();
					patchEditor
						.beginBox(osc_id.c_str(), 1, 1)
							.patching_rect(patching_x, patching_y, 450, 22) // automatic height will be 22 for 'paramOsc' box
							.text(String::FormatC("paramOsc %s", fixupOscAddress(elem.slider3.oscAddress.c_str()).c_str()).c_str())
							.end();
					patching_y += 40;
					
					connect(elem_ids[0], 0, pak_id, 0);
					connect(elem_ids[1], 0, pak_id, 1);
					connect(elem_ids[2], 0, pak_id, 2);
					
					connect(pak_id, 0, osc_id, 0);
					
					connect(osc_id, 0, elem_ids[0], 0);
					connect(osc_id, 0, elem_ids[1], 0);
					connect(osc_id, 0, elem_ids[2], 0);
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
							.patching_rect(patching_x, patching_y, 450, 22) // automatic height will be 22 for 'paramOsc' box
							.text(String::FormatC("paramOsc %s", fixupOscAddress(listbox.oscAddress.c_str()).c_str()).c_str())
							.end();
					patching_y += 40;
				}
				
				const std::string listbox_id = allocObjectId();
				patchEditor
					.beginBox(listbox_id.c_str(), 1, 3)
						.maxclass("live.menu")
						.patching_rect(patching_x, patching_y, 100, 48) // live.dial has a fixed height of 48
						.presentation(true)
						.presentation_rect(elemLayout->x, elemLayout->y, elemLayout->sx, elemLayout->sy)
						.parameter_enable(true)
						.saved_attribute("parameter_mmin", 0)
						.saved_attribute("parameter_mmax", (int)listbox.items.size() - 1)
						.saved_attribute("parameter_initial_enable", listbox.hasDefaultValue)
						.saved_attribute("parameter_initial", defaultIndex)
						.saved_attribute("parameter_longname", elem.name)
						.saved_attribute("parameter_shortname", elem.name)
						.saved_attribute("parameter_enum", listbox.items)
						.saved_attribute("parameter_type", max::kParameterType_Enum)
						.saved_attribute("parameter_linknames", 1)
						.varname(elem.name.c_str())
						.end();
				patching_y += 60;
				
				if (elem.listbox.oscAddress.empty() == false)
				{
					connect(osc_id, 0, listbox_id, 0);
					connect(listbox_id, 0, osc_id, 0);
				}
			}
			else if (elem.type == ControlSurfaceDefinition::kElementType_ColorPicker)
			{
				float defaultH;
				float defaultS;
				float defaultL;
				
				if (elem.colorPicker.colorSpace == ControlSurfaceDefinition::kColorSpace_Hsl)
				{
					defaultH = elem.colorPicker.defaultValue.x;
					defaultS = elem.colorPicker.defaultValue.y;
					defaultL = elem.colorPicker.defaultValue.z;
				}
				else
				{
					rgbToHsl(
						elem.colorPicker.defaultValue.x,
						elem.colorPicker.defaultValue.y,
						elem.colorPicker.defaultValue.z,
						defaultH,
						defaultS,
						defaultL);
				}
				
				defaultS = 1.f; // fixme : perhaps add a separate slider for saturation. and set saturation to 1 when L is 0 or 1
				
				//
				
				const int label_y = elemLayout->y;
				const int label_sy = 20;
				const int colorpicker_y = elemLayout->y + 20;
				const int colorpicker_sy = elemLayout->sy - 20;
				
				patchEditor
					.beginBox(allocObjectId().c_str(), 1, 2)
						.maxclass("comment")
						.text(elem.name.c_str())
						.patching_rect(patching_x, patching_y, 200, 20) // comment will auto-size so size here doesn't really matter, only the position
						.presentation(true)
						.presentation_rect(elemLayout->x, label_y, elemLayout->sx, label_sy)
						.end();
				patching_y += 60;
				
				const std::string swatch_id = allocObjectId();
				patchEditor
					.beginBox(swatch_id.c_str(), 3, 2)
						.maxclass("swatch")
						.patching_rect(patching_x, patching_y, 100, 100)
						.presentation(true)
						.presentation_rect(elemLayout->x, colorpicker_y, elemLayout->sx - 25, colorpicker_sy)
						.parameter_enable(true)
						.saved_attribute("parameter_initial_enable", elem.colorPicker.hasDefaultValue)
						.saved_attribute("parameter_initial", std::vector<float>
							{
								elem.colorPicker.defaultValue.x,
								elem.colorPicker.defaultValue.y,
								elem.colorPicker.defaultValue.z,
								1.f,
								defaultH,
								defaultS,
								defaultL
							})
						.saved_attribute("parameter_longname", elem.name + ".rgb")
						.saved_attribute("parameter_shortname",  elem.colorPicker.displayName)
						.saved_attribute("parameter_type", max::kParameterType_Blob)
						.saved_attribute("parameter_linknames", 1)
						.varname((elem.name + ".rgb").c_str())
						.end();
				patching_y += 100;
				
				const std::string zl_id = allocObjectId();
				patchEditor
					.beginBox(zl_id.c_str(), 2, 2)
						.maxclass("newobj")
						.patching_rect(patching_x, patching_y, 100, 22)
						.text("zl slice 3")
						.end();
				patching_y += 30;
				
				const std::string slider_id = allocObjectId();
				patchEditor
					.beginBox(slider_id.c_str(), 1, 2)
						.maxclass("live.slider")
						.patching_rect(patching_x, patching_y, 20, 100)
						.presentation(true)
						.presentation_rect(elemLayout->x + elemLayout->sx - 20, colorpicker_y, 20, colorpicker_sy)
						.parameter_enable(true)
						.saved_attribute("parameter_initial_enable", elem.colorPicker.hasDefaultValue)
						.saved_attribute("parameter_initial", elem.colorPicker.defaultValue.w)
						.saved_attribute("parameter_mmin", 0.f)
						.saved_attribute("parameter_mmax", 1.f)
						.saved_attribute("parameter_longname", elem.name + ".w")
						.saved_attribute("parameter_shortname", "w")
						.saved_attribute("parameter_type", max::kParameterType_Float)
						.saved_attribute("parameter_unitstyle", max::kUnitStyle_Float)
						.saved_attribute("parameter_linknames", 1)
						.varname((elem.name + ".w").c_str())
						.end();
				patching_y += 100;
				
				const std::string pak_id = allocObjectId();
				patchEditor
					.beginBox(pak_id.c_str(), 3, 2)
						.maxclass("newobj")
						.patching_rect(patching_x, patching_y, 100, 22)
						.parameter_enable(true)
						.text("pak f f f f")
						.end();
				patching_y += 30;
				
				const std::string outputvalues_id = allocObjectId();
				patchEditor
					.beginBox(outputvalues_id.c_str(), 3, 3)
						.maxclass("newobj")
						.patching_rect(patching_x, patching_y, 200, 22)
						.parameter_enable(true)
						.text("sel init outputvalue")
						.end();
				patching_y += 30;
				
				const std::string hsl_id = allocObjectId();
				patchEditor
					.beginBox(hsl_id.c_str(), 3, 3)
						.maxclass("message")
						.patching_rect(patching_x, patching_y, 200, 22)
						.parameter_enable(true)
						.text("hsl 0 1 1")
						.end();
				patching_y += 30;
				
				connect(outputvalues_id, 0, hsl_id, 0);
				connect(hsl_id, 0, swatch_id, 0);
				
				if (elem.colorPicker.oscAddress.empty() == false)
				{
					const std::string osc_id = allocObjectId();
					patchEditor
						.beginBox(osc_id.c_str(), 1, 1)
							.patching_rect(patching_x, patching_y, 450, 22) // automatic height will be 22 for 'paramOsc' box
							.text(String::FormatC("paramOsc %s", fixupOscAddress(elem.colorPicker.oscAddress.c_str()).c_str()).c_str())
							.end();
					patching_y += 40;
					
					connect(swatch_id, 0, zl_id, 0);
					connect(zl_id, 0, pak_id, 0);
					connect(slider_id, 0, pak_id, 3);
					connect(pak_id, 0, osc_id, 0);
				}
			}
			else if (elem.type == ControlSurfaceDefinition::kElementType_Separator)
			{
				auto & separator = elem.separator;
				
				auto box = patchEditor
					.beginBox(allocObjectId().c_str(), 1, 0)
						.maxclass("live.line")
						.patching_rect(patching_x, patching_y, 40, 40)
						.presentation(true)
						.presentation_rect(elemLayout->x, elemLayout->y, elemLayout->sx, elemLayout->sy);
				
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
