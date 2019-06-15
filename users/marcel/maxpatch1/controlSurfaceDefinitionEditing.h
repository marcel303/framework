#pragma once

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
		Surface * surface = nullptr;
		
		SurfaceEditor(Surface * in_surface)
			: surface(in_surface)
		{
		}
		
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
		
		KnobEditor beginKnob(const char * name);
		ListboxEditor beginListbox(const char * name);
		
		SurfaceEditor & endGroup();
	};

	struct KnobEditor
	{
		GroupEditor & groupEditor;
		Knob * knob = nullptr;
		
		KnobEditor(GroupEditor & in_groupEditor, Knob * in_knob)
			: groupEditor(in_groupEditor)
			, knob(in_knob)
		{
		}
		
		KnobEditor & name(const char * name);
		KnobEditor & defaultValue(const float defaultValue);
		KnobEditor & limits(const float min, const float max);
		KnobEditor & exponential(const float exponential);
		KnobEditor & osc(const char * address);
		
		GroupEditor & end();
	};
	
	struct ListboxEditor
	{
		GroupEditor & groupEditor;
		Listbox * listbox = nullptr;
		
		ListboxEditor(GroupEditor & in_groupEditor, Listbox * in_listbox)
			: groupEditor(in_groupEditor)
			, listbox(in_listbox)
		{
		}
		
		ListboxEditor & name(const char * name);
		ListboxEditor & defaultValue(const char * defaultValue);
		ListboxEditor & item(const char * name);
		ListboxEditor & osc(const char * address);
		
		GroupEditor & end();
	};
}
