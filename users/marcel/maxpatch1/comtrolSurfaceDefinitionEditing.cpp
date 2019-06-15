#include "controlSurfaceDefinitionEditing.h"

namespace ControlSurfaceDefinition
{
	GroupEditor SurfaceEditor::pushGroup(const char * name)
	{
		auto * group = new ControlSurfaceDefinition::Group();
		group->name = name;
		surface->groups.push_back(group);

		return GroupEditor(*this, group);
	}
	
	SurfaceLayoutEditor SurfaceEditor::layoutBegin()
	{
		return SurfaceLayoutEditor(*this, &surface->layout);
	}

	//

	KnobEditor GroupEditor::pushKnob(const char * name)
	{
		auto * knob = new ControlSurfaceDefinition::Knob();
		knob->name = name;
		group->elems.push_back(knob);
		
		return KnobEditor(*this, knob);
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