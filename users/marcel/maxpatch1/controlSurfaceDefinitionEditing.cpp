#include "controlSurfaceDefinitionEditing.h"

namespace ControlSurfaceDefinition
{
	GroupEditor SurfaceEditor::pushGroup(const char * name)
	{
		Group group;
		group.name = name;
		surface->groups.push_back(group);

		return GroupEditor(*this, &surface->groups.back());
	}
	
	SurfaceLayoutEditor SurfaceEditor::layoutBegin()
	{
		return SurfaceLayoutEditor(*this, &surface->layout);
	}

	//

	KnobEditor GroupEditor::beginKnob(const char * name)
	{
		Element element;
		element.makeKnob();
		element.knob.name = name;
		group->elems.push_back(element);
		
		return KnobEditor(*this, &group->elems.back().knob);
	}
	
	ListboxEditor GroupEditor::beginListbox(const char * name)
	{
		Element element;
		element.makeListbox();
		element.listbox.name = name;
		group->elems.push_back(element);
		
		return ListboxEditor(*this, &group->elems.back().listbox);
	}

	SurfaceEditor GroupEditor::popGroup()
	{
		return surfaceEditor;
	}

	//

	GroupEditor KnobEditor::end()
	{
		return groupEditor;
	}
	
	//
	
	GroupEditor ListboxEditor::end()
	{
		return groupEditor;
	}
}
