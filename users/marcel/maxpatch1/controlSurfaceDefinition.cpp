#include "controlSurfaceDefinition.h"

namespace ControlSurfaceDefinition
{
	void Surface::initializeDefaultValues()
	{
		for (auto * group : groups)
		{
			for (auto * elem : group->elems)
			{
				if (elem->type == kElementType_Knob)
				{
					auto * knob = static_cast<Knob*>(elem);
					
					if (knob->hasDefaultValue == false)
					{
						knob->defaultValue = knob->min;
						knob->hasDefaultValue = true;
					}
				}
			}
		}
	}
	
	void Surface::performLayout()
	{
		int x = layout.marginX;
		int y = layout.marginY;
		
		for (auto * group : groups)
		{
			int sx = 0;
			
			for (auto * elem : group->elems)
			{
				const bool fits = y + elem->sy + layout.paddingY <= layout.sy - layout.marginY;
				
				if (fits == false)
				{
					x += sx;
					y = layout.marginY;
					
					sx = 0;
				}
				
				elem->x = x;
				elem->y = y;
				
				y += elem->sy + layout.paddingY;
				
				if (elem->sx > sx)
					sx = elem->sx + layout.paddingX;
			}
			
			// end the current group
			
			x += sx;
			y = layout.marginY;
			
			sx = 0;
		}
	}
}
