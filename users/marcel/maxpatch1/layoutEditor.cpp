#include "controlSurfaceDefinition.h"
#include "framework.h"
#include "layoutEditor.h"

namespace ControlSurfaceDefinition
{
	LayoutEditor::LayoutEditor(const Surface * in_surface, SurfaceLayout * in_layout)
		: surface(in_surface)
		, layout(in_layout)
	{
	}
	
	const Element * LayoutEditor::findSurfaceElement(const char * groupName, const char * name) const
	{
		for (auto & group : surface->groups)
			if (group.name == groupName)
				for (auto & elem : group.elems)
					if (elem.name == name)
						return &elem;
		
		return nullptr;
	}
	
	void LayoutEditor::getElementPositionAndSize(const char * groupName, const char * name, int & x, int & y, int & sx, int & sy) const
	{
		x = 0;
		y = 0;
		
		sx = 0;
		sy = 0;
		
		auto * base_layout_element = surface->layout.findElement(groupName, name);
		
		auto * layout_element = layout->findElement(groupName, name);
		
		x = layout_element->hasPosition ? layout_element->x : base_layout_element->x;
		y = layout_element->hasPosition ? layout_element->y : base_layout_element->y;
		
		sx = layout_element->hasSize ? layout_element->sx : base_layout_element->sx;
		sy = layout_element->hasSize ? layout_element->sy : base_layout_element->sy;
	}
	
	void LayoutEditor::snapToSurfaceElements(
		ElementLayout & layout_elem,
		const int snap_direction_x,
		bool & has_snap_x,
		int & snap_x,
		const int snap_direction_y,
		bool & has_snap_y,
		int & snap_y) const
	{
		Assert(layout_elem.hasPosition);
		
		const Element * self = findSurfaceElement(
			layout_elem.groupName.c_str(),
			layout_elem.name.c_str());
		
		auto * base_layout_elem = surface->layout.findElement(
			layout_elem.groupName.c_str(),
			layout_elem.name.c_str());
		
		const int layout_elem_sx = layout_elem.hasSize ? layout_elem.sx : base_layout_elem->sx;
		const int layout_elem_sy = layout_elem.hasSize ? layout_elem.sy : base_layout_elem->sy;
		
		bool has_nearest_dx = false;
		bool has_nearest_dy = false;
		
		int nearest_dx = 0;
		int nearest_dy = 0;
		
		bool * has_nearest_d[2] = { &has_nearest_dx, &has_nearest_dy };
		int * nearest_d[2] = { &nearest_dx, &nearest_dy };
		int * snap[2] = { &snap_x, &snap_y };
		const int * snap_direction[2] = { &snap_direction_x, &snap_direction_y };
		
		// snap with padding
		
		for (auto & group : surface->groups)
		{
			for (auto & elem : group.elems)
			{
				if (&elem == self)
					continue;
				
				const int layout_elem_p1[2] = { layout_elem.x,                  layout_elem.y                  };
				const int layout_elem_p2[2] = { layout_elem.x + layout_elem_sx, layout_elem.y + layout_elem_sy };
				
				int elem_p1[2];
				int elem_p2[2];
				getElementPositionAndSize(group.name.c_str(), elem.name.c_str(), elem_p1[0], elem_p1[1], elem_p2[0], elem_p2[1]);
				elem_p2[0] += elem_p1[0];
				elem_p2[1] += elem_p1[1];
				
				for (int snap_axis = 0; snap_axis < 2; ++snap_axis)
				{
					if (*snap_direction[snap_axis] == 0)
						continue;
					
					const int overlap_axis = 1 - snap_axis;
					
					const bool overlap =
						layout_elem_p1[overlap_axis] < elem_p2[overlap_axis] &&
						layout_elem_p2[overlap_axis] > elem_p1[overlap_axis];
					
					if (overlap == false)
						continue;
					
					const int direction = *snap_direction[snap_axis];
					
					Assert(direction != 0);
					
					{
						const int p1 = direction < 0 ? layout_elem_p1[snap_axis] : layout_elem_p2[snap_axis];
						const int p2 = direction < 0 ? elem_p2[snap_axis] + kPaddingSize : elem_p1[snap_axis] - kPaddingSize;
						
						const int dp = p2 - p1;
						
						if (direction < 0 && dp <= 0)
							continue;
						if (direction > 0 && dp >= 0)
							continue;
						
						if (*has_nearest_d[snap_axis] == false || std::abs(dp) < std::abs(*nearest_d[snap_axis]))
						{
							*has_nearest_d[snap_axis] = true;
							*nearest_d[snap_axis] = dp;
							*snap[snap_axis] = p1 + dp;
						}
					}
				}
			}
		}
	
		if (keyboard.isDown(SDLK_LSHIFT) || keyboard.isDown(SDLK_RSHIFT))
		{
			// snap side to side (top to top, bottom to bottom, etc)
			
			for (auto & group : surface->groups)
			{
				for (auto & elem : group.elems)
				{
					if (&elem == self)
						continue;

					int other_x;
					int other_y;
					int other_sx;
					int other_sy;
					
					getElementPositionAndSize(
						group.name.c_str(),
						elem.name.c_str(),
						other_x,
						other_y,
						other_sx,
						other_sy);
					
					for (int direction = -1; direction <= +1; direction += 2)
					{
						const int x1 = direction == -1 ? other_x : other_x + other_sx;
						const int y1 = direction == -1 ? other_y : other_y + other_sy;
						const int x2 = direction == -1 ? layout_elem.x : layout_elem.x + layout_elem_sx;
						const int y2 = direction == -1 ? layout_elem.y : layout_elem.y + layout_elem_sy;
						
						const int dx = x1 - x2;
						const int dy = y1 - y2;
						
						if (snap_direction_x == direction && (has_nearest_dx == false || std::abs(dx) < std::abs(nearest_dx)))
						{
							has_nearest_dx = true;
							nearest_dx = dx;
							snap_x = x1;
						}
						
						if (snap_direction_y == direction && (has_nearest_dy == false || std::abs(dy) < std::abs(nearest_dy)))
						{
							has_nearest_dy = true;
							nearest_dy = dy;
							snap_y = y1;
						}
					}
				}
			}
		}
		
		has_snap_x = false;
		has_snap_y = false;
		
		if (has_nearest_dx && std::abs(nearest_dx) < kSnapSize)
		{
			Assert(snap_direction_x != 0);
			
			layout_elem.x += nearest_dx;
			has_snap_x = true;
		}
		
		if (has_nearest_dy && std::abs(nearest_dy) < kSnapSize)
		{
			Assert(snap_direction_y != 0);
			
			layout_elem.y += nearest_dy;
			has_snap_y = true;
		}
	}
	
	void LayoutEditor::sizeConstrain(
		ElementLayout & layout_elem_to_skip,
		int & x1,
		int & y1,
		int & x2,
		int & y2,
		const int snap_direction_x, // -1, 0, +1
		bool & has_snap_x,
		int & snap_x,
		const int snap_direction_y, // -1, 0, +1
		bool & has_snap_y,
		int & snap_y) const
	{
		const Element * elem_to_skip = findSurfaceElement(
			layout_elem_to_skip.groupName.c_str(),
			layout_elem_to_skip.name.c_str());
		
		bool has_nearest_dx = false;
		bool has_nearest_dy = false;
		
		int nearest_dx = 0;
		int nearest_dy = 0;
		
		bool * has_nearest_d[2] = { &has_nearest_dx, &has_nearest_dy };
		int * nearest_d[2] = { &nearest_dx, &nearest_dy };
		int * snap[2] = { &snap_x, &snap_y };
		const int * snap_direction[2] = { &snap_direction_x, &snap_direction_y };
		
		// snap with padding
		
		for (auto & group : surface->groups)
		{
			for (auto & elem : group.elems)
			{
				if (&elem == elem_to_skip)
					continue;
				
				const int layout_elem_p1[2] = { x1, y1 };
				const int layout_elem_p2[2] = { x2, y2 };
				
				int elem_p1[2];
				int elem_p2[2];
				getElementPositionAndSize(group.name.c_str(), elem.name.c_str(), elem_p1[0], elem_p1[1], elem_p2[0], elem_p2[1]);
				elem_p2[0] += elem_p1[0];
				elem_p2[1] += elem_p1[1];
				
				for (int snap_axis = 0; snap_axis < 2; ++snap_axis)
				{
					if (*snap_direction[snap_axis] == 0)
						continue;
					
					const int overlap_axis = 1 - snap_axis;
					
					const bool overlap =
						layout_elem_p1[overlap_axis] < elem_p2[overlap_axis] &&
						layout_elem_p2[overlap_axis] > elem_p1[overlap_axis];
					
					if (overlap == false)
						continue;
					
					const int direction = *snap_direction[snap_axis];
					
					{
						const int p1 = direction < 0 ? layout_elem_p1[snap_axis] : layout_elem_p2[snap_axis];
						const int p2 = direction < 0 ? elem_p2[snap_axis] + kPaddingSize : elem_p1[snap_axis] - kPaddingSize;
						
						const int dp = p2 - p1;
						
						if (direction < 0 && dp <= 0)
							continue;
						if (direction > 0 && dp >= 0)
							continue;
						
						if (*has_nearest_d[snap_axis] == false || std::abs(dp) < std::abs(*nearest_d[snap_axis]))
						{
							*has_nearest_d[snap_axis] = true;
							*nearest_d[snap_axis] = dp;
							*snap[snap_axis] = p1 + dp;
						}
					}
				}
			}
		}
	
		if (keyboard.isDown(SDLK_LSHIFT) || keyboard.isDown(SDLK_RSHIFT))
		{
			// snap side to side (top to top, bottom to bottom, etc)
			
			for (auto & group : surface->groups)
			{
				for (auto & elem : group.elems)
				{
					if (&elem == elem_to_skip)
						continue;

					int other_x;
					int other_y;
					int other_sx;
					int other_sy;
					
					getElementPositionAndSize(
						group.name.c_str(),
						elem.name.c_str(),
						other_x,
						other_y,
						other_sx,
						other_sy);
					
					for (int direction = -1; direction <= +1; direction += 2)
					{
						const int px1 = direction == -1 ? other_x : other_x + other_sx;
						const int py1 = direction == -1 ? other_y : other_y + other_sy;
						const int px2 = direction == -1 ? x1 : x2;
						const int py2 = direction == -1 ? y1 : y2;
						
						const int dx = px1 - px2;
						const int dy = py1 - py2;
						
						if (snap_direction_x == direction && (has_nearest_dx == false || std::abs(dx) < std::abs(nearest_dx)))
						{
							has_nearest_dx = true;
							nearest_dx = dx;
							snap_x = px1;
						}
						
						if (snap_direction_y == direction && (has_nearest_dy == false || std::abs(dy) < std::abs(nearest_dy)))
						{
							has_nearest_dy = true;
							nearest_dy = dy;
							snap_y = py1;
						}
					}
				}
			}
		}
		
		has_snap_x = false;
		has_snap_y = false;
		
		if (has_nearest_dx && std::abs(nearest_dx) < kSnapSize)
		{
			Assert(snap_direction_x != 0);
			
			if (snap_direction_x == -1)
				x1 += nearest_dx;
			else
				x2 += nearest_dx;
			
			has_snap_x = true;
		}
		
		if (has_nearest_dy && std::abs(nearest_dy) < kSnapSize)
		{
			Assert(snap_direction_y != 0);
			
			if (snap_direction_y == -1)
				y1 += nearest_dy;
			else
				y2 += nearest_dy;
			
			has_snap_y = true;
		}
	}
	
	void LayoutEditor::dragSize(
		int & elem_x,
		int & elem_sx,
		const int mouse_x,
		const int drag_corner_x,
		const int min_size)
	{
		if (drag_corner_x == -1)
		{
			const int new_x = mouse_x;
			int dx = new_x - elem_x;
			int new_sx = elem_sx - dx;
			if (new_sx < min_size)
				dx += new_sx - min_size;
			elem_x += dx;
			elem_sx -= dx;
		}
		else if (drag_corner_x == +1)
		{
			const int new_x = mouse_x;
			const int dx = new_x - (elem_x + elem_sx);
			elem_sx += dx;
			if (elem_sx < min_size)
				elem_sx = min_size;
		}
		else
		{
			// nothing to do
		}
	}
	
	bool LayoutEditor::tick(const float dt, bool & inputIsCaptured, const bool enableEditing, const bool enableSnapping)
	{
		bool hasChanged = false;
		
		if (inputIsCaptured)
		{
			state = kState_Idle;
			has_snap_x = false;
			has_snap_y = false;
		}
		else if (enableEditing)
		{
			inputIsCaptured |= selected_element != nullptr;
			
			if (mouse.wentDown(BUTTON_LEFT))
			{
				selected_element = nullptr;
				
				for (auto & layout_elem : layout->elems)
				{
					auto * base_layout_elem = surface->layout.findElement(layout_elem.groupName.c_str(), layout_elem.name.c_str());
					
					if (base_layout_elem == nullptr)
						continue;
					
					const int x = layout_elem.hasPosition ? layout_elem.x : base_layout_elem->x;
					const int y = layout_elem.hasPosition ? layout_elem.y : base_layout_elem->y;
					const int sx = layout_elem.hasSize ? layout_elem.sx : base_layout_elem->sx;
					const int sy = layout_elem.hasSize ? layout_elem.sy : base_layout_elem->sy;

					const int dx1 = x - mouse.x;
					const int dx2 = x + sx - mouse.x;
		
					const int dy1 = y - mouse.y;
					const int dy2 = y + sy - mouse.y;
		
					const int t_drag_corner_x = dx1 < 0 && dx1 >= -kCornerSize ? -1 : dx2 >= 0 && dx2 < kCornerSize ? +1 : 0;
					const int t_drag_corner_y = dy1 < 0 && dy1 >= -kCornerSize ? -1 : dy2 >= 0 && dy2 < kCornerSize ? +1 : 0;
				
					if (t_drag_corner_x != 0 && t_drag_corner_y != 0)
					{
						state = kState_DragSize;
						
						selected_element = &layout_elem;
						
						drag_offset_x = t_drag_corner_x == -1 ? dx1 : dx2;
						drag_offset_y = t_drag_corner_y == -1 ? dy1 : dy2;
						
						drag_corner_x = t_drag_corner_x;
						drag_corner_y = t_drag_corner_y;
					}
					else
					{
						const bool isInside =
							mouse.x >= x && mouse.x < x + sx &&
							mouse.y >= y && mouse.y < y + sy;
						
						if (isInside)
						{
							state = kState_DragMove;
							
							selected_element = &layout_elem;
							
							drag_offset_x = mouse.x - x;
							drag_offset_y = mouse.y - y;
							
							drag_direction_x = 0;
							drag_direction_y = 0;
						}
					}
				}
			}
			
			if (selected_element != nullptr)
			{
				if (mouse.wentUp(BUTTON_LEFT))
				{
					state = kState_Idle;
					has_snap_x = false;
					has_snap_y = false;
				}
			}
			
			if (state == kState_DragMove)
			{
				if (mouse.dx != 0 || mouse.dy != 0)
				{
					auto * base_layout_elem = surface->layout.findElement(
						selected_element->groupName.c_str(),
						selected_element->name.c_str());
					
					if (selected_element->hasPosition == false)
					{
						selected_element->hasPosition = true;
						selected_element->x = base_layout_elem->x;
						selected_element->y = base_layout_elem->y;
					}
					
					selected_element->x = mouse.x - drag_offset_x;
					selected_element->y = mouse.y - drag_offset_y;
					
					if (mouse.dx < 0)
						drag_direction_x = -1;
					else if (mouse.dx > 0)
						drag_direction_x = +1;
					
					if (mouse.dy < 0)
						drag_direction_y = -1;
					else if (mouse.dy > 0)
						drag_direction_y = +1;
					
					if (enableSnapping)
					{
						snapToSurfaceElements(
							*selected_element,
							drag_direction_x,
							has_snap_x,
							snap_x,
							drag_direction_y,
							has_snap_y,
							snap_y);
					}
					
					hasChanged = true;
				}
			}
			else if (state == kState_DragSize)
			{
				if (mouse.dx != 0 || mouse.dy != 0)
				{
					auto * base_layout_elem = surface->layout.findElement(
						selected_element->groupName.c_str(),
						selected_element->name.c_str());
					
					if (selected_element->hasPosition == false)
					{
						selected_element->hasPosition = true;
						selected_element->x = base_layout_elem->x;
						selected_element->y = base_layout_elem->y;
					}
					
					if (selected_element->hasSize == false)
					{
						selected_element->hasSize = true;
						selected_element->sx = base_layout_elem->sx;
						selected_element->sy = base_layout_elem->sy;
					}
					
					dragSize(
						selected_element->x,
						selected_element->sx,
						mouse.x + drag_offset_x,
						drag_corner_x,
						40);
					
					dragSize(
						selected_element->y,
						selected_element->sy,
						mouse.y + drag_offset_y,
						drag_corner_y,
						40);
					
					int x1 = selected_element->x;
					int y1 = selected_element->y;
					int x2 = selected_element->x + selected_element->sx;
					int y2 = selected_element->y + selected_element->sy;
					
					sizeConstrain(
						*selected_element,
						x1, y1,
						x2, y2,
						drag_corner_x,
						has_snap_x,
						snap_x,
						drag_corner_y,
						has_snap_y,
						snap_y);
					
					selected_element->x = x1;
					selected_element->y = y1;
					selected_element->sx = x2 - x1;
					selected_element->sy = y2 - y1;
					
					hasChanged = true;
				}
			}
			
			inputIsCaptured |= selected_element != nullptr;
		}
		
		return hasChanged;
	}
	
	void LayoutEditor::drawOverlay(const bool enableEditing) const
	{
		if (has_snap_x)
		{
			setColor(127, 0, 255, 127);
			drawLine(snap_x, 0, snap_x, 1 << 16);
		}
		
		if (has_snap_y)
		{
			setColor(127, 0, 255, 127);
			drawLine(0, snap_y, 1 << 16, snap_y);
		}
		
		if (enableEditing)
		{
			for (auto & layout_elem : layout->elems)
			{
				auto * base_layout_elem = surface->layout.findElement(
					layout_elem.groupName.c_str(),
					layout_elem.name.c_str());
				
				const int x = layout_elem.hasPosition ? layout_elem.x : base_layout_elem->x;
				const int y = layout_elem.hasPosition ? layout_elem.y : base_layout_elem->y;
				const int sx = layout_elem.hasSize ? layout_elem.sx : base_layout_elem->sx;
				const int sy = layout_elem.hasSize ? layout_elem.sy : base_layout_elem->sy;
				
				const int x1 = x;
				const int y1 = y;
				const int x2 = x + sx;
				const int y2 = y + sy;
				
				const bool isSelected = (&layout_elem == selected_element);
				
				if (isSelected)
					setColor(31, 31, 255);
				else
					setColor(127, 0, 255);
				drawRectLine(x1, y1, x2, y2);
				
				const bool isInside =
					mouse.x >= x1 && mouse.x < x2 &&
					mouse.y >= y1 && mouse.y < y2;
				
				if (isInside || (&layout_elem == selected_element))
				{
					if (isSelected)
						setColor(31, 31, 255, 100);
					else
						setColor(127, 0, 255, 100);
					drawRect(
						x1, y1,
						x1 + kCornerSize, y1 + kCornerSize);
					drawRect(
						x2 - kCornerSize, y1,
						x2, y1 + kCornerSize);
					drawRect(
						x2 - kCornerSize, y2 - kCornerSize,
						x2, y2);
					drawRect(
						x1, y2 - kCornerSize,
						x1 + kCornerSize, y2);
				}
			}
		}
	}
}
