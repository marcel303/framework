#include "controlSurfaceDefinition.h"
#include "reflection.h"

namespace ControlSurfaceDefinition
{
	void Surface::initializeDefaultValues()
	{
		for (auto & group : groups)
		{
			for (auto & elem : group.elems)
			{
				if (elem.type == kElementType_Knob)
				{
					auto & knob = elem.knob;
					
					if (knob.hasDefaultValue == false)
					{
						knob.defaultValue = knob.min;
						knob.hasDefaultValue = true;
					}
				}
			}
		}
	}
	
	void Surface::performLayout()
	{
		int x = layout.marginX;
		int y = layout.marginY;
		
		for (auto & group : groups)
		{
			int sx = 0;
			
			for (auto & elem : group.elems)
			{
				const bool fits = y + elem.sy + layout.paddingY <= layout.sy - layout.marginY;
				
				if (fits == false)
				{
					x += sx;
					y = layout.marginY;
					
					sx = 0;
				}
				
				elem.x = x;
				elem.y = y;
				
				y += elem.sy + layout.paddingY;
				
				if (elem.sx > sx)
					sx = elem.sx + layout.paddingX;
			}
			
			// end the current group
			
			x += sx;
			y = layout.marginY;
			
			sx = 0;
		}
	}
	
	//
	
	void reflect(TypeDB & typeDB)
	{
		typeDB.addEnum<ElementType>("ControlSurfaceDefinition::ElementType")
			.add("none", kElementType_None)
			.add("knob", kElementType_Knob)
			.add("listbox", kElementType_Listbox);

		typeDB.addStructured<Knob>("ControlSurfaceDefinition::Knob")
			.add("name", &Knob::name)
			.add("defaultValue", &Knob::defaultValue)
			.add("hasDefaultValue", &Knob::hasDefaultValue)
			.add("min", &Knob::min)
			.add("max", &Knob::max)
			.add("exponential", &Knob::exponential)
			.add("oscAddress", &Knob::oscAddress);
		
		typeDB.addStructured<Listbox>("ControlSurfaceDefinition::Listbox")
			.add("name", &Listbox::name)
			.add("items", &Listbox::items)
			.add("defaultValue", &Listbox::defaultValue)
			.add("hasDefaultValue", &Listbox::hasDefaultValue)
			.add("oscAddress", &Listbox::oscAddress);
		
		typeDB.addStructured<Element>("ControlSurfaceDefinition::Element")
			.add("type", &Element::type)
			.add("x", &Element::x)
			.add("y", &Element::y)
			.add("width", &Element::sx)
			.add("height", &Element::sy)
			.add("knob", &Element::knob)
			.add("listbox", &Element::listbox);
		
		typeDB.addStructured<Group>("ControlSurfaceDefinition::Group")
			.add("name", &Group::name)
			.add("elements", &Group::elems);
	
		typeDB.addStructured<SurfaceLayout>("ControlSurfaceDefinition::SurfaceLayout")
			.add("sx", &SurfaceLayout::sx)
			.add("sy", &SurfaceLayout::sy)
			.add("marginX", &SurfaceLayout::marginX)
			.add("marginY", &SurfaceLayout::marginY)
			.add("paddingX", &SurfaceLayout::paddingX)
			.add("paddingY", &SurfaceLayout::paddingY);
		
		typeDB.addStructured<Surface>("ControlSurfaceDefinition::Surface")
			.add("groups", &Surface::groups)
			.add("layout", &Surface::layout);
	}
}
