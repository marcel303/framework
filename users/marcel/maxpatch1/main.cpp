// max objects includes
#include <string>
#include <vector>

// UI app includes
#include "controlSurfaceDefinition.h"
#include "controlSurfaceDefinitionEditing.h"
#include "framework.h"
#include "osc/OscOutboundPacketStream.h"
#include "oscSender.h"
#include "Path.h"
#include "reflection-bindtofile.h"
#include "StringEx.h"

// Max/MSP generator app includes
#include "framework.h"
#include "maxPatchFromControlSurfaceDefinition.h"
#include "maxPatchEditor.h"
#include "reflection-jsonio.h"
#include <stdio.h> // FILE

// live UI includes
#include "liveUi.h"

// ++++ : move UI generator to a separate source file
// ++++ : refine UI generator
// ++++ : add reflection type UI structure

// ++++ : move Max/MSP patch generator to its own source file
// todo : determine a way to include Max/MSP patch snippets, so complicated stuff can be generated externally

// todo : create UI app, which reads reflected UI structure from file and allows knob control, OSC output and creating presets

//

#define ENABLE_LAYOUT_EDITOR 1

#if ENABLE_LAYOUT_EDITOR

namespace ControlSurfaceDefinition
{
	struct Layout
	{
		std::vector<ElementLayout> elems;
		
		ElementLayout * addElem(const char * groupName, const char * name)
		{
			elems.push_back(ElementLayout());
			auto & elem = elems.back();
			elem.groupName = groupName;
			elem.name = name;
			return &elem;
		}
		
		const ElementLayout * findElem(const char * groupName, const char * name) const
		{
			for (auto & elem : elems)
				if (elem.groupName == groupName && elem.name == name)
					return &elem;
			
			return nullptr;
		}
		
		static void reflect(TypeDB & typeDB)
		{
			typeDB.addStructured<ControlSurfaceDefinition::Layout>("ControlSurfaceDefinition::Layout")
				.add("elems", &Layout::elems);
		}
	};
	
	struct LayoutEditor
	{
		static const int kCornerSize = 7;
		static const int kSnapSize = 8;
		static const int kPaddingSize = 4;
		
		enum State
		{
			kState_Idle,
			kState_DragMove,
			kState_DragSize
		};
		
		const Surface * surface = nullptr;
		
		Layout * layout = nullptr;
		
		State state = kState_Idle;
		
		ElementLayout * selected_element = nullptr;
		int drag_offset_x = 0; // for positioning and sizing
		int drag_offset_y = 0;
		int drag_direction_x = 0;
		int drag_direction_y = 0;
		int drag_corner_x = 0; // for sizing only
		int drag_corner_y = 0;
		
		bool has_snap_x = false;
		bool has_snap_y = false;
		int snap_x = 0;
		int snap_y = 0;
		
		LayoutEditor(const Surface * in_surface, Layout * in_layout)
			: surface(in_surface)
			, layout(in_layout)
		{
		}
		
		const Element * findSurfaceElement(const char * groupName, const char * name) const
		{
			for (auto & group : surface->groups)
				if (group.name == groupName)
					for (auto & elem : group.elems)
						if (elem.name == name)
							return &elem;
			
			return nullptr;
		}
		
		void snapToSurfaceElements(
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
			
			const int layout_elem_sx = layout_elem.hasSize ? layout_elem.sx : self->initialSx;
			const int layout_elem_sy = layout_elem.hasSize ? layout_elem.sy : self->initialSy;
			
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
					
					auto * elem_layout = surface->layout.findElementLayout(group.name.c_str(), elem.name.c_str());

					const int layout_elem_p1[2] = { layout_elem.x,                  layout_elem.y                  };
					const int layout_elem_p2[2] = { layout_elem.x + layout_elem_sx, layout_elem.y + layout_elem_sy };
					
					const int elem_p1[2] = { elem_layout->x,                   elem_layout->y                   };
					const int elem_p2[2] = { elem_layout->x + elem_layout->sx, elem_layout->y + elem_layout->sy };
					
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
						
						auto * elem_layout = surface->layout.findElementLayout(group.name.c_str(), elem.name.c_str());
						
						for (int direction = -1; direction <= +1; direction += 2)
						{
							const int x1 = direction == -1 ? elem_layout->x : elem_layout->x + elem_layout->sx;
							const int y1 = direction == -1 ? elem_layout->y : elem_layout->y + elem_layout->sy;
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
		
		void sizeConstrain(
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
					
					auto * elem_layout = surface->layout.findElementLayout(group.name.c_str(), elem.name.c_str());
					
					const int layout_elem_p1[2] = { x1, y1 };
					const int layout_elem_p2[2] = { x2, y2 };
					
					const int elem_p1[2] = { elem_layout->x,                   elem_layout->y                   };
					const int elem_p2[2] = { elem_layout->x + elem_layout->sx, elem_layout->y + elem_layout->sy };
					
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
						
						auto * elem_layout = surface->layout.findElementLayout(group.name.c_str(), elem.name.c_str());
						
						for (int direction = -1; direction <= +1; direction += 2)
						{
							const int px1 = direction == -1 ? elem_layout->x : elem_layout->x + elem_layout->sx;
							const int py1 = direction == -1 ? elem_layout->y : elem_layout->y + elem_layout->sy;
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
		
		static void dragSize(
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
		
		bool tick(const float dt, bool & inputIsCaptured, const bool enableEditing, const bool enableSnapping)
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
						auto * base_layout_elem = surface->layout.findElementLayout(layout_elem.groupName.c_str(), layout_elem.name.c_str());
						
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
						auto * base_layout_elem = surface->layout.findElementLayout(
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
						auto * base_layout_elem = surface->layout.findElementLayout(
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
						
						if (enableSnapping)
						{
							//snapToSurfaceElements(*selected_element, has_snap_x, snap_x, has_snap_y, snap_y);
						}
						
						hasChanged = true;
					}
				}
				
				inputIsCaptured |= selected_element != nullptr;
			}
			
			return hasChanged;
		}
		
		void drawOverlay(const bool enableEditing) const
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
					auto * base_layout_elem = surface->layout.findElementLayout(
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
	};
}

struct Button
{
	int x = 0;
	int y = 0;
	int sx = 0;
	int sy = 0;
	
	std::string text;
	
	bool isToggle = false;
	bool toggleValue = false;
	
	bool hover = false;
	bool isDown = false;
	bool isClicked = false;
	
	void makeButton(
		const char * in_text,
		const int in_x,
		const int in_y,
		const int in_sx,
		const int in_sy)
	{
		text = in_text;
		
		x = in_x;
		y = in_y;
		sx = in_sx;
		sy = in_sy;
	}
	
	void makeToggle(
		const char * in_text,
		const bool in_toggleValue,
		const int in_x,
		const int in_y,
		const int in_sx,
		const int in_sy)
	{
		text = in_text;
		
		isToggle = true;
		toggleValue = in_toggleValue;
		
		x = in_x;
		y = in_y;
		sx = in_sx;
		sy = in_sy;
	}
	
	bool tick(bool & inputIsCaptured)
	{
		isClicked = false;
		
		const bool isInside =
			mouse.x >= x && mouse.x < x + sx &&
			mouse.y >= y && mouse.y < y + sy;
		
		hover = false;
		
		if (inputIsCaptured)
			isDown = false;
		else
		{
			hover = isInside;
			
			if (isDown)
			{
				inputIsCaptured = true;
				
				if (mouse.wentUp(BUTTON_LEFT))
				{
					isDown = false;
					isClicked = true;
					if (isToggle)
						toggleValue = !toggleValue;
				}
			}
			else
			{
				if (isInside && mouse.wentDown(BUTTON_LEFT))
				{
					inputIsCaptured = true;
					isDown = true;
				}
			}
			
			if (isInside)
			{
				inputIsCaptured = true;
			}
		}
		
		return isClicked;
	}
	
	void draw() const
	{
		const float hue = .5f;
		const float sat = .2f;
		const ::Color colorDown = ::Color::fromHSL(hue, sat, .5f);
		const ::Color colorSelected = ::Color::fromHSL(hue, sat, .4f);
		const ::Color colorDeselected = ::Color::fromHSL(hue, sat, .6f);
		const ::Color borderColorNormal = ::Color::fromHSL(hue, sat, .2f);
		const ::Color borderColorHover = ::Color::fromHSL(hue, sat, .9f);
		
		if (isToggle)
			setColor(isDown ? colorDown : toggleValue ? colorSelected : colorDeselected);
		else
			setColor(isDown ? colorSelected : colorDeselected);
		hqBegin(HQ_FILLED_ROUNDED_RECTS);
		{
			hqFillRoundedRect(x, y, x + sx, y + sy, 4);
		}
		hqEnd();
		
		setColor(hover ? borderColorHover : borderColorNormal);
		hqBegin(HQ_STROKED_ROUNDED_RECTS);
		{
			hqStrokeRoundedRect(x, y, x + sx, y + sy, 4, 2);
		}
		hqEnd();
		
		setColor(colorWhite);
		drawText(x + sx/2, y + sy/2, 12, 0, 0, "%s", text.c_str());
	}
};

struct LayoutEditorView
{
	Button btn_editToggle;
	Button btn_snapToggle;
	Button btn_save;
	Button btn_load;
	Button btn_layout;
	Button btn_reset;
	
	LayoutEditorView()
	{
		btn_editToggle.makeToggle("edit", true, 10, 10, 70, 20);
		btn_snapToggle.makeToggle("snap", true, 10, 40, 70, 20);
		btn_save.makeButton("save", 10, 70, 70, 20);
		btn_load.makeButton("load", 10, 100, 70, 20);
		btn_layout.makeButton("layout", 10, 130, 70, 20);
		btn_reset.makeButton("reset", 10, 160, 70, 20);
	}
	
	void tick(bool & inputIsCaptured)
	{
		btn_editToggle.tick(inputIsCaptured);
		btn_snapToggle.tick(inputIsCaptured);
	
		btn_save.tick(inputIsCaptured); // todo : buttons shouldn't really be a member of the layout editor directly
		btn_load.tick(inputIsCaptured);
		btn_layout.tick(inputIsCaptured);
		btn_reset.tick(inputIsCaptured);
	}
	
	void drawOverlay() const
	{
		btn_editToggle.draw();
		btn_snapToggle.draw();
		btn_save.draw();
		btn_load.draw();
		btn_layout.draw();
		btn_reset.draw();
	}
};

#endif

int main(int arg, char * argv[])
{
	changeDirectory(CHIBI_RESOURCE_PATH);
	
	TypeDB typeDB;
	
	typeDB.addPlain<bool>("bool", kDataType_Bool);
	typeDB.addPlain<int>("int", kDataType_Int);
	typeDB.addPlain<float>("float", kDataType_Float);
	typeDB.addPlain<Vec2>("Vec2", kDataType_Float2);
	typeDB.addPlain<Vec3>("Vec3", kDataType_Float3);
	typeDB.addPlain<Vec4>("Vec4", kDataType_Float4);
	typeDB.addPlain<std::string>("string", kDataType_String);
	
	max::reflect(typeDB);
	
	ControlSurfaceDefinition::reflect(typeDB);
	
	ControlSurfaceDefinition::Layout::reflect(typeDB);
	
	// create control surface definition
	
	ControlSurfaceDefinition::Surface surface;
	
	ControlSurfaceDefinition::SurfaceEditor surfaceEditor(&surface);

	surfaceEditor
		.beginGroup("master")
			.beginLabel("master")
				.divideBottom()
				.size(100, 40)
				.end()
			.beginKnob("intensity")
				.defaultValue(5.f)
				.limits(0.f, 100.f)
				.exponential(2.f)
				.unit(ControlSurfaceDefinition::kUnit_Percentage)
				.osc("/master/intensity")
				.end()
			.beginKnob("VU")
				.limits(0.f, 1.f)
				.exponential(2.f)
				.unit(ControlSurfaceDefinition::kUnit_Time)
				.osc("/master/duration")
				.end()
			.beginSeparator()
				.borderColor(1.f, .5f, .25f, 1.f)
				.end()
			.beginKnob("A/B")
				.limits(0.f, 1.f)
				.exponential(2.f)
				.unit(ControlSurfaceDefinition::kUnit_Float)
				.osc("/master/ab")
				.end()
			.beginListbox("mode")
				.item("a")
				.item("b")
				.defaultValue("b")
				.osc("/master/mode")
				.end()
			.beginColorPicker("color")
				.colorSpace(ControlSurfaceDefinition::kColorSpace_Rgbw)
				.defaultValue(1.f, 0.f, 0.f, 1.f)
				.osc("/master/color")
				.end()
			.endGroup();
	
	for (int i = 0; i < 7; ++i)
	{
		surfaceEditor
			.beginGroup(String::FormatC("source %d", i + 1).c_str())
				.beginLabel("source")
					.divideBottom()
					.end()
				.beginKnob("position")
					.limits(0.f, 1.f)
					.exponential(2.f)
					.end()
				.beginKnob("speed")
					.limits(0.f, 1.f)
					.exponential(2.f)
					.defaultValue(.3f)
					.end()
				.beginKnob("scale")
					.limits(0.f, 1.f)
					.exponential(2.f)
					.defaultValue(.2f)
					.end()
				.beginKnob("position2")
					.limits(0.f, 1.f)
					.exponential(2.f)
					.end()
				.beginKnob("speed2")
					.limits(0.f, 10.f)
					.exponential(2.f)
					.defaultValue(8.f)
					.end()
				.beginKnob("scale2")
					.limits(0.f, 1.f)
					.exponential(2.f)
					.end()
				.separator()
				.endGroup();
	}
	
	surface.initializeNames();
	surface.initializeDefaultValues();
	surface.initializeDisplayNames();
	
	surfaceEditor.beginLayout()
		.size(800, 200)
		.margin(10, 10)
		.padding(4, 4)
		.end();
	
	saveObjectToFile(typeDB, typeDB.findType(surface), &surface, "surface-definition.json");
	
	// live UI app from control surface definition
	
	framework.init(800, 200);
	
	LiveUi liveUi;
	
	std::string currentFilename;
	
#if ENABLE_LAYOUT_EDITOR
	ControlSurfaceDefinition::Layout layout;
	
	ControlSurfaceDefinition::LayoutEditor layoutEditor(&surface, &layout);
	
	LayoutEditorView layoutEditorView;
#endif

	auto loadLiveUi = [&](const char * filename)
	{
		ControlSurfaceDefinition::Surface newSurface;
		
		if (loadObjectFromFile(typeDB, typeDB.findType(newSurface), &newSurface, filename))
		{
			currentFilename = filename;
			
			surface = newSurface;
			
			surface.initializeNames();
			surface.initializeDefaultValues();
			surface.initializeDisplayNames();
			
			ControlSurfaceDefinition::SurfaceEditor surfaceEditor(&surface);
			
			// recreate the live ui
			
			liveUi = LiveUi();

			for (auto & group : surface.groups)
			{
				for (auto & elem : group.elems)
				{
					auto * elemLayout = surface.layout.findElementLayout(group.name.c_str(), elem.name.c_str());
					
					liveUi.addElem(&elem, elemLayout);
				}
			}
			
			liveUi
				.osc("127.0.0.1", 2000)
				.osc("127.0.0.1", 2002);
			
		#if ENABLE_LAYOUT_EDITOR
			// recreate the layout editor
			
			layout = ControlSurfaceDefinition::Layout();
			for (auto & group : surface.groups)
				for (auto & elem : group.elems)
					if (elem.name.empty() == false)
						layout.addElem(group.name.c_str(), elem.name.c_str());
			
			layoutEditor = ControlSurfaceDefinition::LayoutEditor(&surface, &layout);
		#endif
		}
	};
	
	auto updateLiveUiWithLayout = [](LiveUi & liveUi, const ControlSurfaceDefinition::Surface & surface, const ControlSurfaceDefinition::Layout & layout)
	{
		// patch live UI with information from layout
		
		for (auto & group : surface.groups)
		{
			for (auto & elem : group.elems)
			{
				auto * base_elem_layout = surface.layout.findElementLayout(group.name.c_str(), elem.name.c_str());
				
				auto * elem_layout = layout.findElem(group.name.c_str(), elem.name.c_str());
				
				auto * ui_elem = liveUi.findElem(&elem);
				
				if (elem_layout->hasPosition)
				{
					ui_elem->x = elem_layout->x;
					ui_elem->y = elem_layout->y;
				}
				else if (base_elem_layout->hasPosition)
				{
					ui_elem->x = base_elem_layout->x;
					ui_elem->y = base_elem_layout->y;
				}
				else
				{
					ui_elem->x = 0;
					ui_elem->y = 0;
				}
				
				if (elem_layout->hasSize)
				{
					ui_elem->sx = elem_layout->sx;
					ui_elem->sy = elem_layout->sy;
				}
				else if (base_elem_layout->hasSize)
				{
					ui_elem->sx = base_elem_layout->sx;
					ui_elem->sy = base_elem_layout->sy;
				}
				else
				{
					ui_elem->sx = 0;
					ui_elem->sy = 0;
				}
			}
		}
	};
	
	loadLiveUi("surface-definition.json");
	
	for (;;)
	{
		framework.waitForEvents = true;
		
		framework.process();
		
		if (framework.quitRequested)
			break;
		
		for (auto & filename : framework.droppedFiles)
		{
			loadLiveUi(filename.c_str());
		}
		
		bool inputIsCaptured = false;
		
	#if ENABLE_LAYOUT_EDITOR
		layoutEditorView.tick(inputIsCaptured);
		
		if (layoutEditor.tick(
			framework.timeStep,
			inputIsCaptured,
			layoutEditorView.btn_editToggle.toggleValue,
			layoutEditorView.btn_snapToggle.toggleValue))
		{
			updateLiveUiWithLayout(liveUi, surface, layout);
		}
		
		if (layoutEditorView.btn_save.isClicked)
		{
			const std::string layout_filename = Path::ReplaceExtension(currentFilename, "layout.json");
			if (saveObjectToFile(typeDB, layout, layout_filename.c_str()) == false)
				logError("failed to save layout to file");
		}
		
		if (layoutEditorView.btn_load.isClicked)
		{
			const std::string layout_filename = Path::ReplaceExtension(currentFilename, "layout.json");
			ControlSurfaceDefinition::Layout new_layout;
			
			if (loadObjectFromFile(typeDB, new_layout, layout_filename.c_str()) == false)
				logError("failed to load layout from file");
			else
			{
				layout = new_layout;
				
				updateLiveUiWithLayout(liveUi, surface, layout);
			}
		}
		
		if (layoutEditorView.btn_layout.isClicked)
		{
			// perform layout on the control surface elements
			
			surfaceEditor.beginLayout()
				.size(800, 200)
				.margin(10, 10)
				.padding(4, 4)
				.end();
			
			// re-apply layout overrides
			
			updateLiveUiWithLayout(liveUi, surface, layout);
		}
		
		if (layoutEditorView.btn_reset.isClicked)
		{
			layout = ControlSurfaceDefinition::Layout();
			
		// todo : layout editor should automatically populate layout ?
			for (auto & group : surface.groups)
				for (auto & elem : group.elems)
					if (elem.name.empty() == false)
						layout.addElem(group.name.c_str(), elem.name.c_str());
			
			layoutEditor = ControlSurfaceDefinition::LayoutEditor(&surface, &layout);
			
			// re-apply layout overrides
			
			updateLiveUiWithLayout(liveUi, surface, layout);
		}
	#endif
	
		liveUi.tick(framework.timeStep, inputIsCaptured);
	
		const int c = 160;
		
		framework.beginDraw(c/2, c/2, c/2, 0);
		{
			setFont("calibri.ttf");
			
			setLumi(c);
			setAlpha(255);
			drawRect(0, 0, surface.layout.sx, surface.layout.sy);
			
			liveUi.draw();
			
		#if ENABLE_LAYOUT_EDITOR
			layoutEditor.drawOverlay(layoutEditorView.btn_editToggle.toggleValue);
			
			layoutEditorView.drawOverlay();
		#endif
			
			liveUi.drawTooltip();
		}
		framework.endDraw();
	}

	// generate Max patch from control surface definition
	
	max::Patch patch;
	
	if (maxPatchFromControlSurfaceDefinition(surface, patch) == false)
	{
		logError("failed to generate Max patch from control surface definition");
		return -1;
	}
	else
	{
		// serialize Max patch to json and write the json text to file
		
		rapidjson::StringBuffer buffer;
		REFLECTIONIO_JSON_WRITER json_writer(buffer);
		
		if (object_tojson_recursive(typeDB, typeDB.findType(patch), &patch, json_writer) == false)
		{
			logError("failed to serialize patch to json");
			return -1;
		}
		else
		{
			FILE * f = fopen("test.maxpat", "wt");
			
			if (f == nullptr)
			{
				logError("failed to open file for write");
				return -1;
			}
			else
			{
				if (fputs(buffer.GetString(), f) < 0)
				{
					logError("failed to write json text to file");
				}
				else
				{
					logInfo("written Max patch to file successfully [length: %d bytes]", (int)ftell(f));
				}
				
				fclose(f);
				f = nullptr;
			}
		}
	}
	
	return 0;
}
