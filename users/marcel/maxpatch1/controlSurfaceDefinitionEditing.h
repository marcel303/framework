#pragma once

#include "controlSurfaceDefinition.h"

// todo : remove ControlSurfaceDefinition::

namespace ControlSurfaceDefinition
{
	struct Group;
	struct Knob;
	struct Listbox;
	struct Surface;
	struct SurfaceLayout;
	
	//
	
	struct GroupEditor;
	struct KnobEditor;
	struct ListboxEditor;
	struct SurfaceEditor;
	struct SurfaceLayoutEditor;

	struct SurfaceEditor
	{
		ControlSurfaceDefinition::Surface * surface = nullptr;
		
		SurfaceEditor(ControlSurfaceDefinition::Surface * in_surface)
			: surface(in_surface)
		{
		}
		
		GroupEditor pushGroup(const char * name);
		
		SurfaceLayoutEditor layoutBegin();
	};

	struct SurfaceLayoutEditor
	{
		SurfaceEditor surfaceEditor;
		ControlSurfaceDefinition::SurfaceLayout * layout = nullptr;
		
		SurfaceLayoutEditor(SurfaceEditor & in_surfaceEditor, ControlSurfaceDefinition::SurfaceLayout * in_layout)
			: surfaceEditor(in_surfaceEditor)
			, layout(in_layout)
		{
		}
		
		SurfaceLayoutEditor & size(const int sx, const int sy)
		{
			layout->sx = sx;
			layout->sy = sy;
			return *this;
		}
		
		SurfaceLayoutEditor & margin(const int x, const int y)
		{
			layout->marginX = x;
			layout->marginY = y;
			return *this;
		}
		
		SurfaceLayoutEditor & padding(const int x, const int y)
		{
			layout->paddingX = x;
			layout->paddingY = y;
			return *this;
		}
		
		SurfaceEditor & layoutEnd()
		{
			surfaceEditor.surface->performLayout();
			return surfaceEditor;
		}
	};
	
	struct GroupEditor
	{
		SurfaceEditor surfaceEditor;
		ControlSurfaceDefinition::Group * group = nullptr;
		
		GroupEditor(const SurfaceEditor & in_surfaceEditor, ControlSurfaceDefinition::Group * in_group)
			: surfaceEditor(in_surfaceEditor)
			, group(in_group)
		{
		}
		
		KnobEditor beginKnob(const char * name);
		ListboxEditor beginListbox(const char * name);
		
		SurfaceEditor popGroup();
		
		void name(const char * name)
		{
			group->name = name;
		}
	};

	struct KnobEditor
	{
		GroupEditor groupEditor;
		ControlSurfaceDefinition::Knob * knob = nullptr;
		
		KnobEditor(const GroupEditor & in_groupEditor, ControlSurfaceDefinition::Knob * in_knob)
			: groupEditor(in_groupEditor)
			, knob(in_knob)
		{
		}
		
		KnobEditor & name(const char * name)
		{
			knob->name = name;
			return *this;
		}
		
		KnobEditor & defaultValue(const float defaultValue)
		{
			knob->defaultValue = defaultValue;
			knob->hasDefaultValue = true;
			return *this;
		}
		
		KnobEditor & limits(const float min, const float max)
		{
			knob->min = min;
			knob->max = max;
			return *this;
		}
		
		KnobEditor & exponential(const float exponential)
		{
			knob->exponential = exponential;
			return *this;
		}
		
		KnobEditor & osc(const char * address)
		{
			knob->oscAddress = address;
			return *this;
		}
		
		GroupEditor end();
	};
	
	struct ListboxEditor
	{
		GroupEditor groupEditor;
		ControlSurfaceDefinition::Listbox * listbox = nullptr;
		
		ListboxEditor(const GroupEditor & in_groupEditor, ControlSurfaceDefinition::Listbox * in_listbox)
			: groupEditor(in_groupEditor)
			, listbox(in_listbox)
		{
		}
		
		ListboxEditor & name(const char * name)
		{
			listbox->name = name;
			return *this;
		}
		
		ListboxEditor & defaultValue(const char * defaultValue)
		{
			listbox->defaultValue = defaultValue;
			listbox->hasDefaultValue = true;
			return *this;
		}
		
		ListboxEditor & item(const char * name)
		{
			listbox->items.push_back(name);
			return *this;
		}
		
		ListboxEditor & osc(const char * address)
		{
			listbox->oscAddress = address;
			return *this;
		}
		
		GroupEditor end();
	};
}
