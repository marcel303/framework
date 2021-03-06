#include "controlSurfaceDefinitionEditing.h"
#include "controlSurfaceDefinition.h"

namespace ControlSurfaceDefinition
{
	SurfaceEditor & SurfaceEditor::name(const char * name)
	{
		surface->name = name;
		return *this;
	}
	
	GroupEditor SurfaceEditor::beginGroup(const char * name)
	{
		Group group;
		group.name = name;
		surface->groups.push_back(group);

		return GroupEditor(*this, &surface->groups.back());
	}
	
	SurfaceLayoutEditor SurfaceEditor::beginLayout()
	{
		return SurfaceLayoutEditor(*this, &surface->layout);
	}
	
	//
	
	SurfaceLayoutEditor & SurfaceLayoutEditor::size(const int sx, const int sy)
	{
		layout->sx = sx;
		layout->sy = sy;
		return *this;
	}

	SurfaceLayoutEditor & SurfaceLayoutEditor::margin(const int x, const int y)
	{
		layout->marginX = x;
		layout->marginY = y;
		return *this;
	}

	SurfaceLayoutEditor & SurfaceLayoutEditor::padding(const int x, const int y)
	{
		layout->paddingX = x;
		layout->paddingY = y;
		return *this;
	}

	SurfaceEditor & SurfaceLayoutEditor::end()
	{
		surfaceEditor.surface->performLayout();
		return surfaceEditor;
	}

	//

	void GroupEditor::name(const char * name)
	{
		group->name = name;
	}
	
	LabelEditor GroupEditor::beginLabel(const char * text)
	{
		Element element;
		element.makeLabel();
		element.label.text = text;
		group->elems.push_back(element);
		
		return LabelEditor(*this, &group->elems.back(), &group->elems.back().label);
	}
	
	KnobEditor GroupEditor::beginKnob(const char * name)
	{
		Element element;
		element.makeKnob();
		element.name = name;
		group->elems.push_back(element);
		
		return KnobEditor(*this, &group->elems.back(), &group->elems.back().knob);
	}
	
	ButtonEditor GroupEditor::beginButton(const char * name)
	{
		Element element;
		element.makeButton();
		element.name = name;
		group->elems.push_back(element);
		
		return ButtonEditor(*this, &group->elems.back(), &group->elems.back().button);
	}
	
	Slider2Editor GroupEditor::beginSlider2(const char * name)
	{
		Element element;
		element.makeSlider2();
		element.name = name;
		group->elems.push_back(element);
		
		return Slider2Editor(*this, &group->elems.back(), &group->elems.back().slider2);
	}
	
	Slider3Editor GroupEditor::beginSlider3(const char * name)
	{
		Element element;
		element.makeSlider3();
		element.name = name;
		group->elems.push_back(element);
		
		return Slider3Editor(*this, &group->elems.back(), &group->elems.back().slider3);
	}
	
	ListboxEditor GroupEditor::beginListbox(const char * name)
	{
		Element element;
		element.makeListbox();
		element.name = name;
		group->elems.push_back(element);
		
		return ListboxEditor(*this, &group->elems.back(), &group->elems.back().listbox);
	}
	
	ColorPickerEditor GroupEditor::beginColorPicker(const char * name)
	{
		Element element;
		element.makeColorPicker();
		element.name = name;
		group->elems.push_back(element);
		
		return ColorPickerEditor(*this, &group->elems.back(), &group->elems.back().colorPicker);
	}
	
	SeparatorEditor GroupEditor::beginSeparator()
	{
		Element element;
		element.makeSeparator();
		group->elems.push_back(element);
		
		return SeparatorEditor(*this, &group->elems.back(), &group->elems.back().separator);
	}
	
	GroupEditor & GroupEditor::label(const char * text)
	{
		Element element;
		element.makeLabel();
		element.label.text = text;
		group->elems.push_back(element);
		
		return *this;
	}

	GroupEditor & GroupEditor::separator()
	{
		Element element;
		element.makeSeparator();
		group->elems.push_back(element);
		
		return *this;
	}
	
	SurfaceEditor & GroupEditor::endGroup()
	{
		return surfaceEditor;
	}
	
	//
	
	template <typename T>
	T & ElementEditor<T>::name(const char * name)
	{
		element->name = name;
		return static_cast<T&>(*this);
	}
	
	template <typename T>
	T & ElementEditor<T>::size(const int sx, const int sy)
	{
		element->initialSx = sx;
		element->initialSy = sy;
		return static_cast<T&>(*this);
	}
	
	template <typename T>
	T & ElementEditor<T>::divideBottom()
	{
		element->divideBottom = true;
		return static_cast<T&>(*this);
	}

	template <typename T>
	T & ElementEditor<T>::divideLeft()
	{
		element->divideLeft = true;
		return static_cast<T&>(*this);
	}

	template <typename T>
	T & ElementEditor<T>::divideRight()
	{
		element->divideRight = true;
		return static_cast<T&>(*this);
	}
	
	//
	
	LabelEditor::LabelEditor(GroupEditor & in_groupEditor, Element * in_element, Label * in_label)
		: ElementEditor(in_element)
		, groupEditor(in_groupEditor)
		, label(in_label)
	{
	}
	
	LabelEditor & LabelEditor::text(const char * text)
	{
		label->text = text;
		return *this;
	}
	
	GroupEditor & LabelEditor::end()
	{
		return groupEditor;
	}

	//
	
	KnobEditor & KnobEditor::displayName(const char * displayName)
	{
		knob->displayName = displayName;
		return *this;
	}
	
	KnobEditor & KnobEditor::defaultValue(const float defaultValue)
	{
		knob->defaultValue = defaultValue;
		knob->hasDefaultValue = true;
		return *this;
	}

	KnobEditor & KnobEditor::limits(const float min, const float max)
	{
		knob->min = min;
		knob->max = max;
		return *this;
	}

	KnobEditor & KnobEditor::exponential(const float exponential)
	{
		knob->exponential = exponential;
		return *this;
	}
	
	KnobEditor & KnobEditor::unit(const Unit unit)
	{
		knob->unit = unit;
		return *this;
	}

	KnobEditor & KnobEditor::osc(const char * address)
	{
		knob->oscAddress = address;
		return *this;
	}

	GroupEditor & KnobEditor::end()
	{
		return groupEditor;
	}
	
	//
	
	ButtonEditor & ButtonEditor::displayName(const char * displayName)
	{
		button->displayName = displayName;
		return *this;
	}
	
	ButtonEditor & ButtonEditor::defaultValue(const bool defaultValue)
	{
		button->defaultValue = defaultValue;
		button->hasDefaultValue = true;
		return *this;
	}
	
	ButtonEditor & ButtonEditor::isToggle(const bool isToggle)
	{
		button->isToggle = isToggle;
		return *this;
	}

	ButtonEditor & ButtonEditor::osc(const char * address)
	{
		button->oscAddress = address;
		return *this;
	}

	GroupEditor & ButtonEditor::end()
	{
		return groupEditor;
	}
	
	//
	
	Slider2Editor & Slider2Editor::displayName(const char * displayName)
	{
		slider->displayName = displayName;
		return *this;
	}
	
	Slider2Editor & Slider2Editor::defaultValue(const Vector2 & defaultValue)
	{
		slider->defaultValue = defaultValue;
		slider->hasDefaultValue = true;
		return *this;
	}

	Slider2Editor & Slider2Editor::limits(const Vector2 & min, const Vector2 & max)
	{
		slider->min = min;
		slider->max = max;
		return *this;
	}
	
	Slider2Editor & Slider2Editor::exponential(const float exponential)
	{
		slider->exponential.set(exponential, exponential);
		return *this;
	}
	
	Slider2Editor & Slider2Editor::exponential(const Vector2 & exponential)
	{
		slider->exponential = exponential;
		return *this;
	}

	Slider2Editor & Slider2Editor::osc(const char * address)
	{
		slider->oscAddress = address;
		return *this;
	}

	GroupEditor & Slider2Editor::end()
	{
		return groupEditor;
	}
	
	//
	
	Slider3Editor & Slider3Editor::displayName(const char * displayName)
	{
		slider->displayName = displayName;
		return *this;
	}
	
	Slider3Editor & Slider3Editor::defaultValue(const Vector3 & defaultValue)
	{
		slider->defaultValue = defaultValue;
		slider->hasDefaultValue = true;
		return *this;
	}

	Slider3Editor & Slider3Editor::limits(const Vector3 & min, const Vector3 & max)
	{
		slider->min = min;
		slider->max = max;
		return *this;
	}
	
	Slider3Editor & Slider3Editor::exponential(const float exponential)
	{
		slider->exponential.set(exponential, exponential, exponential);
		return *this;
	}
	
	Slider3Editor & Slider3Editor::exponential(const Vector3 & exponential)
	{
		slider->exponential = exponential;
		return *this;
	}

	Slider3Editor & Slider3Editor::osc(const char * address)
	{
		slider->oscAddress = address;
		return *this;
	}

	GroupEditor & Slider3Editor::end()
	{
		return groupEditor;
	}
	
	//
	
	ListboxEditor & ListboxEditor::defaultValue(const char * defaultValue)
	{
		listbox->defaultValue = defaultValue;
		listbox->hasDefaultValue = true;
		return *this;
	}

	ListboxEditor & ListboxEditor::item(const char * name)
	{
		listbox->items.push_back(name);
		return *this;
	}

	ListboxEditor & ListboxEditor::osc(const char * address)
	{
		listbox->oscAddress = address;
		return *this;
	}
	
	GroupEditor & ListboxEditor::end()
	{
		return groupEditor;
	}
	
	//
	
	ColorPickerEditor & ColorPickerEditor::displayName(const char * displayName)
	{
		colorPicker->displayName = displayName;
		return *this;
	}

	ColorPickerEditor & ColorPickerEditor::colorSpace(const ColorSpace colorSpace)
	{
		colorPicker->colorSpace = colorSpace;
		return *this;
	}
	
	ColorPickerEditor & ColorPickerEditor::defaultValue(const Vector4 & defaultValue)
	{
		colorPicker->defaultValue = defaultValue;
		colorPicker->hasDefaultValue = true;
		return *this;
	}
	
	ColorPickerEditor & ColorPickerEditor::defaultValue(const float r, const float g, const float b, const float a)
	{
		colorPicker->defaultValue = Vector4(r, g, b, a);
		colorPicker->hasDefaultValue = true;
		return *this;
	}

	ColorPickerEditor & ColorPickerEditor::osc(const char * address)
	{
		colorPicker->oscAddress = address;
		return *this;
	}
	
	GroupEditor & ColorPickerEditor::end()
	{
		return groupEditor;
	}
	
	//
	
	SeparatorEditor & SeparatorEditor::borderColor(const float r, const float g, const float b, const float a)
	{
		separator->borderColor.set(r, g, b, a);
		separator->hasBorderColor = true;
		return *this;
	}
	
	SeparatorEditor & SeparatorEditor::thickness(const int thickness)
	{
		separator->thickness = thickness;
		return *this;
	}
	
	GroupEditor & SeparatorEditor::end()
	{
		return groupEditor;
	}
	
	//
	
	template struct ElementEditor<ButtonEditor>;
	template struct ElementEditor<ColorPickerEditor>;
	template struct ElementEditor<KnobEditor>;
	template struct ElementEditor<LabelEditor>;
	template struct ElementEditor<ListboxEditor>;
	template struct ElementEditor<SeparatorEditor>;
	template struct ElementEditor<Slider2Editor>;
	template struct ElementEditor<Slider3Editor>;
}
