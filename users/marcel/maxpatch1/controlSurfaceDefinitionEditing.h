#pragma once

#include "controlSurfaceDefinition.h" // Unit

namespace ControlSurfaceDefinition
{
	struct Button;
	struct Element;
	struct Group;
	struct Knob;
	struct Label;
	struct Listbox;
	struct Surface;
	struct SurfaceLayout;
	
	//
	
	struct ButtonEditor;
	struct ColorPickerEditor;
	struct GroupEditor;
	struct KnobEditor;
	struct LabelEditor;
	struct ListboxEditor;
	struct SeparatorEditor;
	struct Slider2Editor;
	struct Slider3Editor;
	struct SurfaceEditor;
	struct SurfaceLayoutEditor;

	struct SurfaceEditor
	{
		Surface * surface = nullptr;
		
		SurfaceEditor(Surface * in_surface)
			: surface(in_surface)
		{
		}
		
		SurfaceEditor & name(const char * name);
		
		GroupEditor beginGroup(const char * name);
		
		SurfaceLayoutEditor beginLayout();
	};

	struct SurfaceLayoutEditor
	{
		SurfaceEditor & surfaceEditor;
		SurfaceLayout * layout = nullptr;
		
		SurfaceLayoutEditor(SurfaceEditor & in_surfaceEditor, SurfaceLayout * in_layout)
			: surfaceEditor(in_surfaceEditor)
			, layout(in_layout)
		{
		}
		
		SurfaceLayoutEditor & size(const int sx, const int sy);
		SurfaceLayoutEditor & margin(const int x, const int y);
		SurfaceLayoutEditor & padding(const int x, const int y);
		
		SurfaceEditor & end();
	};
	
	struct GroupEditor
	{
		SurfaceEditor & surfaceEditor;
		Group * group = nullptr;
		
		GroupEditor(SurfaceEditor & in_surfaceEditor, Group * in_group)
			: surfaceEditor(in_surfaceEditor)
			, group(in_group)
		{
		}
		
		void name(const char * name);
		
		LabelEditor beginLabel(const char * text);
		KnobEditor beginKnob(const char * name);
		ButtonEditor beginButton(const char * name);
		Slider2Editor beginSlider2(const char * name);
		Slider3Editor beginSlider3(const char * name);
		ListboxEditor beginListbox(const char * name);
		ColorPickerEditor beginColorPicker(const char * name);
		SeparatorEditor beginSeparator();
		
		GroupEditor & label(const char * text);
		GroupEditor & separator();
		
		SurfaceEditor & endGroup();
	};

	template <typename T>
	struct ElementEditor
	{
		Element * element = nullptr;
		
		ElementEditor(Element * in_element)
			: element(in_element)
		{
		}
		
		T & name(const char * name);
		
		T & size(const int sx, const int sy);
		
		T & divideBottom();
		T & divideLeft();
		T & divideRight();
	};
	
	struct LabelEditor : ElementEditor<LabelEditor>
	{
		GroupEditor & groupEditor;
		Label * label = nullptr;
		
		LabelEditor(GroupEditor & in_groupEditor, Element * in_element, Label * in_label);
		
		LabelEditor & text(const char * text);
		
		GroupEditor & end();
	};
	
	struct KnobEditor : ElementEditor<KnobEditor>
	{
		GroupEditor & groupEditor;
		Knob * knob = nullptr;
		
		KnobEditor(GroupEditor & in_groupEditor, Element * in_element, Knob * in_knob)
			: ElementEditor(in_element)
			, groupEditor(in_groupEditor)
			, knob(in_knob)
		{
		}
		
		KnobEditor & displayName(const char * displayName);
		KnobEditor & defaultValue(const float defaultValue);
		KnobEditor & limits(const float min, const float max);
		KnobEditor & exponential(const float exponential);
		KnobEditor & unit(const Unit unit);
		KnobEditor & osc(const char * address);
		
		GroupEditor & end();
	};
	
	struct ButtonEditor : ElementEditor<ButtonEditor>
	{
		GroupEditor & groupEditor;
		Button * button = nullptr;
		
		ButtonEditor(GroupEditor & in_groupEditor, Element * in_element, Button * in_button)
			: ElementEditor(in_element)
			, groupEditor(in_groupEditor)
			, button(in_button)
		{
		}
		
		ButtonEditor & displayName(const char * displayName);
		ButtonEditor & defaultValue(const bool defaultValue);
		ButtonEditor & isToggle(const bool isToggle);
		ButtonEditor & osc(const char * address);
		
		GroupEditor & end();
	};
	
	struct Slider2Editor : ElementEditor<Slider2Editor>
	{
		GroupEditor & groupEditor;
		Slider2 * slider = nullptr;
		
		Slider2Editor(GroupEditor & in_groupEditor, Element * in_element, Slider2 * in_slider)
			: ElementEditor(in_element)
			, groupEditor(in_groupEditor)
			, slider(in_slider)
		{
		}
		
		Slider2Editor & displayName(const char * displayName);
		Slider2Editor & defaultValue(const Vector2 & defaultValue);
		Slider2Editor & limits(const Vector2 & min, const Vector2 & max);
		Slider2Editor & exponential(const float exponential);
		Slider2Editor & exponential(const Vector2 & exponential);
		Slider2Editor & osc(const char * address);
		
		GroupEditor & end();
	};
	
	struct Slider3Editor : ElementEditor<Slider3Editor>
	{
		GroupEditor & groupEditor;
		Slider3 * slider = nullptr;
		
		Slider3Editor(GroupEditor & in_groupEditor, Element * in_element, Slider3 * in_slider)
			: ElementEditor(in_element)
			, groupEditor(in_groupEditor)
			, slider(in_slider)
		{
		}
		
		Slider3Editor & displayName(const char * displayName);
		Slider3Editor & defaultValue(const Vector3 & defaultValue);
		Slider3Editor & limits(const Vector3 & min, const Vector3 & max);
		Slider3Editor & exponential(const float exponential);
		Slider3Editor & exponential(const Vector3 & exponential);
		Slider3Editor & osc(const char * address);
		
		GroupEditor & end();
	};
	
	struct ListboxEditor : ElementEditor<ListboxEditor>
	{
		GroupEditor & groupEditor;
		Listbox * listbox = nullptr;
		
		ListboxEditor(GroupEditor & in_groupEditor, Element * in_element, Listbox * in_listbox)
			: ElementEditor(in_element)
			, groupEditor(in_groupEditor)
			, listbox(in_listbox)
		{
		}
		
		ListboxEditor & defaultValue(const char * defaultValue);
		ListboxEditor & item(const char * name);
		ListboxEditor & osc(const char * address);
		
		GroupEditor & end();
	};
	
	struct ColorPickerEditor : ElementEditor<ColorPickerEditor>
	{
		GroupEditor & groupEditor;
		ColorPicker * colorPicker = nullptr;
		
		ColorPickerEditor(GroupEditor & in_groupEditor, Element * in_element, ColorPicker * in_colorPicker)
			: ElementEditor(in_element)
			, groupEditor(in_groupEditor)
			, colorPicker(in_colorPicker)
		{
		}
		
		ColorPickerEditor & displayName(const char * displayName);
		ColorPickerEditor & colorSpace(const ColorSpace colorSpace);
		ColorPickerEditor & defaultValue(const Vector4 & defaultValue);
		ColorPickerEditor & defaultValue(const float r, const float g, const float b, const float a = 1.f);
		ColorPickerEditor & osc(const char * address);
		
		GroupEditor & end();
	};
	
	struct SeparatorEditor : ElementEditor<SeparatorEditor>
	{
		GroupEditor & groupEditor;
		Separator * separator = nullptr;
		
		SeparatorEditor(GroupEditor & in_groupEditor, Element * in_element, Separator * in_separator)
			: ElementEditor(in_element)
			, groupEditor(in_groupEditor)
			, separator(in_separator)
		{
		}
		
		SeparatorEditor & borderColor(const float r, const float g, const float b, const float a);
		SeparatorEditor & thickness(const int thickness);
		
		GroupEditor & end();
	};
}
