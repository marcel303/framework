#include "controlSurfaceDefinitionEditing.h"
#include "controlSurfaceDefinition.h"

namespace ControlSurfaceDefinition
{
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
		element.knob.name = name;
		group->elems.push_back(element);
		
		return KnobEditor(*this, &group->elems.back(), &group->elems.back().knob);
	}
	
	ListboxEditor GroupEditor::beginListbox(const char * name)
	{
		Element element;
		element.makeListbox();
		element.listbox.name = name;
		group->elems.push_back(element);
		
		return ListboxEditor(*this, &group->elems.back(), &group->elems.back().listbox);
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
	T & ElementEditor<T>::size(const int sx, const int sy)
	{
		element->sx = sx;
		element->sy = sy;
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
	
	KnobEditor & KnobEditor::name(const char * name)
	{
		knob->name = name;
		return *this;
	}
	
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
	
	ListboxEditor & ListboxEditor::name(const char * name)
	{
		listbox->name = name;
		return *this;
	}

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
	
	template struct ElementEditor<LabelEditor>;
	template struct ElementEditor<KnobEditor>;
	template struct ElementEditor<ListboxEditor>;
}
