// max objects includes
#include <string>
#include <vector>

// UI app includes
#include "controlSurfaceDefinition.h"
#include "controlSurfaceDefinitionEditing.h"
#include "framework.h"
#include "osc/OscOutboundPacketStream.h"
#include "oscSender.h"
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
	struct LayoutElement
	{
		std::string groupName;
		std::string name;
		
		bool hasPosition = false;
		int x, y;
		
		bool hasSize = false;
		int sx, sy;
	};
	
	struct Layout
	{
		std::vector<LayoutElement> elems;
		
		LayoutElement * addElem(const char * groupName, const char * name)
		{
			elems.push_back(LayoutElement());
			auto & elem = elems.back();
			elem.groupName = groupName;
			elem.name = name;
			return &elem;
		}
	};
	
	struct LayoutEditor
	{
		static const int kCornerSize = 7;
		static const int kSnapSize = 10;
		
		enum State
		{
			kState_Idle,
			kState_DragMove,
			kState_DragSize
		};
		
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
				bool isClicked = false;
				
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
		
		const Surface * surface = nullptr;
		
		Layout * layout = nullptr;
		
		State state = kState_Idle;
		
		LayoutElement * selected_element = nullptr;
		int drag_offset_x = 0; // for positioning and sizing
		int drag_offset_y = 0;
		int drag_corner_x = 0; // for sizing only
		int drag_corner_y = 0;
		
		bool has_snap_x = false;
		bool has_snap_y = false;
		int snap_x = 0;
		int snap_y = 0;
		
		Button btn_editToggle;
		Button btn_snapToggle;
		
		LayoutEditor(const Surface * in_surface, Layout * in_layout)
			: surface(in_surface)
			, layout(in_layout)
		{
			btn_editToggle.makeToggle("edit", true, 10, 10, 70, 20);
			btn_snapToggle.makeToggle("snap", true, 10, 40, 70, 20);
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
			LayoutElement & layout_elem,
			bool & has_snap_x,
			int & snap_x,
			bool & has_snap_y,
			int & snap_y)
		{
			Assert(layout_elem.hasPosition);
			
			const Element * self = findSurfaceElement(
				layout_elem.groupName.c_str(),
				layout_elem.name.c_str());
			
			const int layout_elem_sx = layout_elem.hasSize ? layout_elem.sx : self->sx;
			const int layout_elem_sy = layout_elem.hasSize ? layout_elem.sy : self->sy;
			
			bool has_nearest_dx = false;
			bool has_nearest_dy = false;
			
			int nearest_dx = 0;
			int nearest_dy = 0;
			
			bool * has_nearest_d[2] = { &has_nearest_dx, &has_nearest_dy };
			int * nearest_d[2] = { &nearest_dx, &nearest_dy };
			int * snap[2] = { &snap_x, &snap_y };
			
			// snap with padding
			
			for (auto & group : surface->groups)
			{
				for (auto & elem : group.elems)
				{
					if (&elem == self)
						continue;
					
					const int layout_elem_p1[2] = { layout_elem.x,                  layout_elem.y                  };
					const int layout_elem_p2[2] = { layout_elem.x + layout_elem_sx, layout_elem.y + layout_elem_sy };
					
					const int elem_p1[2] = { elem.x,           elem.y           };
					const int elem_p2[2] = { elem.x + elem.sx, elem.y + elem.sy };
					
					for (int snap_axis = 0; snap_axis < 2; ++snap_axis)
					{
						const int overlap_axis = 1 - snap_axis;
						
						const bool overlap =
							layout_elem_p1[overlap_axis] < elem_p2[overlap_axis] &&
							layout_elem_p2[overlap_axis] > elem_p1[overlap_axis];
						
						if (overlap == false)
							continue;
						
						for (int direction = -1; direction <= +1; direction += 2)
						{
							const int kPadding = 4;
							
							const int p1 = direction < 0 ? layout_elem_p1[snap_axis] : layout_elem_p2[snap_axis];
							const int p2 = direction < 0 ? elem_p2[snap_axis] + kPadding : elem_p1[snap_axis] - kPadding;
							
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
						
						for (int s1 = 0; s1 < 2; ++s1)
						{
							const int s2 = s1;
							
							const int x1 = elem.x + s1 * elem.sx;
							const int y1 = elem.y + s1 * elem.sy;
							const int x2 = layout_elem.x + layout_elem_sx * s2;
							const int y2 = layout_elem.y + layout_elem_sy * s2;
							
							const int dx = x1 - x2;
							const int dy = y1 - y2;
							
							if (has_nearest_dx == false || std::abs(dx) < std::abs(nearest_dx))
							{
								has_nearest_dx = true;
								nearest_dx = dx;
								snap_x = x1;
							}
							
							if (has_nearest_dy == false || std::abs(dy) < std::abs(nearest_dy))
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
				layout_elem.x += nearest_dx;
				has_snap_x = true;
			}
			
			if (has_nearest_dy && std::abs(nearest_dy) < kSnapSize)
			{
				layout_elem.y += nearest_dy;
				has_snap_y = true;
			}
		}
		
		static void dragSizeAndConstrain(
			int & elem_x,
			int & elem_sx,
			const int mouse_x,
			const int drag_corner_x,
			const int min_size)
		{
			if (drag_corner_x == 0)
			{
				const int new_x = mouse_x;
				int dx = new_x - elem_x;
				int new_sx = elem_sx - dx;
				if (new_sx < min_size)
					dx += new_sx - min_size;
				elem_x += dx;
				elem_sx -= dx;
			}
			else if (drag_corner_x == 1)
			{
				const int new_x = mouse_x;
				const int dx = new_x - (elem_x + elem_sx);
				elem_sx += dx;
				if (elem_sx < min_size)
					elem_sx = min_size;
			}
		}
		
		bool tick(const float dt, bool & inputIsCaptured)
		{
			bool hasChanged = false;
			
			btn_editToggle.tick(inputIsCaptured);
			btn_snapToggle.tick(inputIsCaptured);
			
			if (inputIsCaptured)
			{
				state = kState_Idle;
				has_snap_x = false;
				has_snap_y = false;
			}
			else if (btn_editToggle.toggleValue)
			{
				inputIsCaptured |= selected_element != nullptr;
				
				if (mouse.wentDown(BUTTON_LEFT))
				{
					selected_element = nullptr;
					
					for (auto & layout_elem : layout->elems)
					{
						auto * surface_elem = findSurfaceElement(
							layout_elem.groupName.c_str(),
							layout_elem.name.c_str());
						
						if (surface_elem == nullptr)
							continue;
						
						const int x = layout_elem.hasPosition ? layout_elem.x : surface_elem->x;
						const int y = layout_elem.hasPosition ? layout_elem.y : surface_elem->y;
						const int sx = layout_elem.hasSize ? layout_elem.sx : surface_elem->sx;
						const int sy = layout_elem.hasSize ? layout_elem.sy : surface_elem->sy;

						const int dx1 = x - mouse.x;
						const int dx2 = x + sx - mouse.x;
			
						const int dy1 = y - mouse.y;
						const int dy2 = y + sy - mouse.y;
			
						const int t_drag_corner_x = dx1 < 0 && dx1 >= -kCornerSize ? 0 : dx2 >= 0 && dx2 < kCornerSize ? 1 : 2;
						const int t_drag_corner_y = dy1 < 0 && dy1 >= -kCornerSize ? 0 : dy2 >= 0 && dy2 < kCornerSize ? 1 : 2;
					
						if (t_drag_corner_x != 2 && t_drag_corner_y != 2)
						{
							state = kState_DragSize;
							
							selected_element = &layout_elem;
							
							drag_offset_x = t_drag_corner_x == 0 ? dx1 : dx2;
							drag_offset_y = t_drag_corner_y == 0 ? dy1 : dy2;
							
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
						auto * surface_elem = findSurfaceElement(
							selected_element->groupName.c_str(),
							selected_element->name.c_str());

						if (selected_element->hasPosition == false)
						{
							selected_element->hasPosition = true;
							selected_element->x = surface_elem->x;
							selected_element->y = surface_elem->y;
						}
						
						selected_element->x = mouse.x - drag_offset_x;
						selected_element->y = mouse.y - drag_offset_y;
						
						if (btn_snapToggle.toggleValue)
						{
							snapToSurfaceElements(*selected_element, has_snap_x, snap_x, has_snap_y, snap_y);
						}
						
						hasChanged = true;
					}
				}
				else if (state == kState_DragSize)
				{
					if (mouse.dx != 0 || mouse.dy != 0)
					{
						auto * surface_elem = findSurfaceElement(
							selected_element->groupName.c_str(),
							selected_element->name.c_str());
						
						if (selected_element->hasPosition == false)
						{
							selected_element->hasPosition = true;
							selected_element->x = surface_elem->x;
							selected_element->y = surface_elem->y;
						}
						
						if (selected_element->hasSize == false)
						{
							selected_element->hasSize = true;
							selected_element->sx = surface_elem->sx;
							selected_element->sy = surface_elem->sy;
						}
						
						dragSizeAndConstrain(
							selected_element->x,
							selected_element->sx,
							mouse.x + drag_offset_x,
							drag_corner_x,
							40);
						
						dragSizeAndConstrain(
							selected_element->y,
							selected_element->sy,
							mouse.y + drag_offset_y,
							drag_corner_y,
							40);
						
						if (btn_snapToggle.toggleValue)
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
		
		void drawOverlay() const
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
			
			if (btn_editToggle.toggleValue)
			{
				for (auto & layout_elem : layout->elems)
				{
					auto * surface_elem = findSurfaceElement(
						layout_elem.groupName.c_str(),
						layout_elem.name.c_str());
					
					const int x = layout_elem.hasPosition ? layout_elem.x : surface_elem->x;
					const int y = layout_elem.hasPosition ? layout_elem.y : surface_elem->y;
					const int sx = layout_elem.hasSize ? layout_elem.sx : surface_elem->sx;
					const int sy = layout_elem.hasSize ? layout_elem.sy : surface_elem->sy;
					
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
			
			btn_editToggle.draw();
			btn_snapToggle.draw();
		}
	};
}

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
	
	saveObjectToFile(&typeDB, typeDB.findType(surface), &surface, "surface-definition.json");
	
	// live UI app from control surface definition
	
	framework.init(800, 200);
	
	LiveUi liveUi;
	
#if ENABLE_LAYOUT_EDITOR
	ControlSurfaceDefinition::Layout layout;
	for (auto & group : surface.groups)
		for (auto & elem : group.elems)
			if (elem.name.empty() == false)
				layout.addElem(group.name.c_str(), elem.name.c_str());
	
	ControlSurfaceDefinition::LayoutEditor layoutEditor(&surface, &layout);
#endif
	
	for (auto & group : surface.groups)
	{
		for (auto & elem : group.elems)
		{
			liveUi.addElem(&elem);
		}
	}
	
	liveUi
		.osc("127.0.0.1", 2000)
		.osc("127.0.0.1", 2002);
	
	for (;;)
	{
		framework.waitForEvents = true;
		
		framework.process();
		
		if (framework.quitRequested)
			break;
		
		for (auto & filename : framework.droppedFiles)
		{
			ControlSurfaceDefinition::Surface newSurface;
			
			if (loadObjectFromFile(&typeDB, typeDB.findType(newSurface), &newSurface, filename.c_str()))
			{
				surface = newSurface;
				
				surface.initializeNames();
				surface.initializeDefaultValues();
				surface.initializeDisplayNames();
				
				ControlSurfaceDefinition::SurfaceEditor surfaceEditor(&surface);
				
				surfaceEditor.beginLayout()
					.size(800, 200)
					.margin(10, 10)
					.padding(4, 4)
					.end();
				
				// recreate the live ui
				
				liveUi = LiveUi();
	
				for (auto & group : surface.groups)
				{
					for (auto & elem : group.elems)
					{
						liveUi.addElem(&elem);
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
		}
		
		bool inputIsCaptured = false;
		
	#if ENABLE_LAYOUT_EDITOR
		if (layoutEditor.tick(framework.timeStep, inputIsCaptured))
		{
			// patch control surface without information from layout
			
			for (auto & layout_elem : layout.elems)
			{
				auto * surface_elem = layoutEditor.findSurfaceElement(
					layout_elem.groupName.c_str(),
					layout_elem.name.c_str());
				
				if (layout_elem.hasPosition)
				{
					const_cast<ControlSurfaceDefinition::Element*>(surface_elem)->x = layout_elem.x;
					const_cast<ControlSurfaceDefinition::Element*>(surface_elem)->y = layout_elem.y;
				}
				
				if (layout_elem.hasSize)
				{
					const_cast<ControlSurfaceDefinition::Element*>(surface_elem)->sx = layout_elem.sx;
					const_cast<ControlSurfaceDefinition::Element*>(surface_elem)->sy = layout_elem.sy;
				}
			}
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
			layoutEditor.drawOverlay();
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
