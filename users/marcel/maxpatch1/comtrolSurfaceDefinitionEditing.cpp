#include "controlSurfaceDefinitionEditing.h"

namespace ControlSurfaceDefinition
{
	GroupEditor SurfaceEditor::pushGroup(const char * name)
	{
		ControlSurfaceDefinition::Group group;
		group.name = name;
		surface->groups.push_back(group);

		return GroupEditor(*this, &surface->groups.back());
	}
	
	SurfaceLayoutEditor SurfaceEditor::layoutBegin()
	{
		return SurfaceLayoutEditor(*this, &surface->layout);
	}

	//

	KnobEditor GroupEditor::pushKnob(const char * name)
	{
		ControlSurfaceDefinition::Element element;
		element.makeKnob();
		element.knob.name = name;
		group->elems.push_back(element);
		
		return KnobEditor(*this, &group->elems.back().knob);
	}

	SurfaceEditor GroupEditor::popGroup()
	{
		return surfaceEditor;
	}

	//

	GroupEditor KnobEditor::popKnob()
	{
		return groupEditor;
	}
}
