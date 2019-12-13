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

#include "layoutEditor.h"

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
	
	framework.init(1200, 200);
	
	LiveUi liveUi;
	
	std::string currentFilename;
	
#if ENABLE_LAYOUT_EDITOR
	ControlSurfaceDefinition::SurfaceLayout layout;
	
	ControlSurfaceDefinition::LayoutEditor layoutEditor(&surface, &layout);
	
	LayoutEditorView layoutEditorView;
#endif

	auto loadLiveUi = [&](const char * filename) -> bool
	{
		bool result = false;
		
		ControlSurfaceDefinition::Surface newSurface;
		
		if (loadObjectFromFile(typeDB, typeDB.findType(newSurface), &newSurface, filename))
		{
			result = true;
			
			framework.getCurrentWindow().setTitle(filename);
			
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
					liveUi.addElem(&elem);
				}
			}
			
			liveUi
				.osc("127.0.0.1", 2000)
				.osc("192.168.1.106", 2000)
				.osc("127.0.0.1", 2002);
			
			const ControlSurfaceDefinition::SurfaceLayout * layouts[] = { &surface.layout };
			liveUi.applyLayouts(surface, layouts, 1);
			
		#if ENABLE_LAYOUT_EDITOR
			// recreate the layout editor
			
			layout = ControlSurfaceDefinition::SurfaceLayout();
			for (auto & group : surface.groups)
				for (auto & elem : group.elems)
					if (elem.name.empty() == false)
						layout.addElement(group.name.c_str(), elem.name.c_str());
			
			layoutEditor = ControlSurfaceDefinition::LayoutEditor(&surface, &layout);
		#endif
		}
		
		return result;
	};
	
	auto updateLiveUiWithLayout = [](LiveUi & liveUi, const ControlSurfaceDefinition::Surface & surface, const ControlSurfaceDefinition::SurfaceLayout & layout)
	{
		// patch live UI with information from layout
		
		const ControlSurfaceDefinition::SurfaceLayout * layouts[] =
		{
			&surface.layout,
			&layout
		};
		
		liveUi.applyLayouts(surface, layouts, sizeof(layouts) / sizeof(layouts[0]));
	};
	
	auto loadLayout = [&](const char * filename)
	{
		ControlSurfaceDefinition::SurfaceLayout new_layout;
	
		if (loadObjectFromFile(typeDB, new_layout, filename) == false)
			logError("failed to load layout from file");
		else
		{
			layout = new_layout;
			
		// todo : layout editor should automatically populate layout ?
			for (auto & group : surface.groups)
				for (auto & elem : group.elems)
					if (layout.findElement(group.name.c_str(), elem.name.c_str()) == nullptr)
						layout.addElement(group.name.c_str(), elem.name.c_str());
		
			updateLiveUiWithLayout(liveUi, surface, layout);
		}
	};
	
	if (loadLiveUi("surface-definition.json"))
	{
		const std::string layout_filename = Path::ReplaceExtension(currentFilename, "layout.json");
		loadLayout(layout_filename.c_str());
	}
	
	float ui_x = 0.f;
	float ui_y = 0.f;
	
	bool isAnimating = false;
	
	for (;;)
	{
		framework.waitForEvents = (isAnimating == false);
		
		framework.process();
		
		if (framework.quitRequested)
			break;
		
		isAnimating = false;
		
		for (auto & filename : framework.droppedFiles)
		{
			if (loadLiveUi(filename.c_str()))
			{
				const std::string layout_filename = Path::ReplaceExtension(currentFilename, "layout.json");
				loadLayout(layout_filename.c_str());
			}
		}
		
		bool inputIsCaptured = false;
		
	#if ENABLE_LAYOUT_EDITOR
		layoutEditorView.tick(inputIsCaptured);
		
		if (layoutEditorView.btn_save.isClicked)
		{
			const std::string layout_filename = Path::ReplaceExtension(currentFilename, "layout.json");
			if (saveObjectToFile(typeDB, layout, layout_filename.c_str()) == false)
				logError("failed to save layout to file");
		}
		
		if (layoutEditorView.btn_load.isClicked)
		{
			const std::string layout_filename = Path::ReplaceExtension(currentFilename, "layout.json");
			loadLayout(layout_filename.c_str());
		}
		
		if (layoutEditorView.btn_layout.isClicked)
		{
			// perform layout on the control surface elements
			
			surface.performLayout();
			
			// re-apply layout overrides
			
			updateLiveUiWithLayout(liveUi, surface, layout);
		}
		
		if (layoutEditorView.btn_reset.isClicked)
		{
			layout = ControlSurfaceDefinition::SurfaceLayout();
			
		// todo : layout editor should automatically populate layout ?
			for (auto & group : surface.groups)
				for (auto & elem : group.elems)
					if (layout.findElement(group.name.c_str(), elem.name.c_str()) == nullptr)
						layout.addElement(group.name.c_str(), elem.name.c_str());
			
			layoutEditor = ControlSurfaceDefinition::LayoutEditor(&surface, &layout);
			
			// re-apply layout overrides
			
			updateLiveUiWithLayout(liveUi, surface, layout);
		}
	#endif
	
		const int desired_ui_x = keyboard.isDown(SDLK_l) ? 240 : 120; // fixme : remove this animation hack
		const int desired_ui_y = 10;
		
		if (framework.waitForEvents == false)
		{
			ui_x = lerp<float>(desired_ui_x, ui_x, powf(.5f, framework.timeStep * 60.f));
			ui_y = lerp<float>(desired_ui_y, ui_y, powf(.5f, framework.timeStep * 60.f));
		}
		
		if (fabsf(ui_x - desired_ui_x) < .02f &&
			fabsf(ui_y - desired_ui_y) < .02f)
		{
			ui_x = desired_ui_x;
			ui_y = desired_ui_y;
		}
		else
		{
			isAnimating = true;
		}
		
		pushScroll(ui_x, ui_y);
		{
		#if ENABLE_LAYOUT_EDITOR
			if (layoutEditor.tick(
				framework.timeStep,
				inputIsCaptured,
				layoutEditorView.btn_editToggle.toggleValue,
				layoutEditorView.btn_snapToggle.toggleValue))
			{
				updateLiveUiWithLayout(liveUi, surface, layout);
			}
		#endif
		
			liveUi.tick(framework.timeStep, inputIsCaptured);
		}
		popScroll();
	
		const int c = 160;
		
		framework.beginDraw(c/2, c/2, c/2, 0);
		{
			int viewSx, viewSy;
			framework.getCurrentViewportSize(viewSx, viewSy);
			
			setFont("calibri.ttf");
			
			pushScroll(ui_x, ui_y);
			{
				setLumi(c);
				setAlpha(255);
				int surfaceSx = 0;
				int surfaceSy = 0;
				for (auto & group : surface.groups)
				{
					for (auto & elem : group.elems)
					{
						auto * base_layout_element = surface.layout.findElement(group.name.c_str(), elem.name.c_str());
			
						auto * layout_element = layout.findElement(group.name.c_str(), elem.name.c_str());
						
						const int x = layout_element->hasPosition ? layout_element->x : base_layout_element->x;
						const int y = layout_element->hasPosition ? layout_element->y : base_layout_element->y;
						
						const int sx = layout_element->hasSize ? layout_element->sx : base_layout_element->sx;
						const int sy = layout_element->hasSize ? layout_element->sy : base_layout_element->sy;
			
						if (x + sx > surfaceSx)
							surfaceSx = x + sx;
						if (y + sy > surfaceSy)
							surfaceSy = y + sy;
					}
				}
				surfaceSx += surface.layout.marginX;
				surfaceSy += surface.layout.marginY;
				
				hqSetGradient(GRADIENT_RADIAL, Mat4x4(true).Scale(1.f / 1000.f).Translate(-300, 0, 0), Color::fromHSL(.5f, .5f, .5f), Color::fromHSL(.2f, .5f, .5f), COLOR_IGNORE);
				hqBegin(HQ_FILLED_ROUNDED_RECTS);
				hqFillRoundedRect(0, 0, surfaceSx, surfaceSy, 4);
				hqEnd();
				hqClearGradient();
				//drawRect(0, 0, surfaceSx, surfaceSy);
				
				liveUi.draw();

			#if ENABLE_LAYOUT_EDITOR
				layoutEditor.drawOverlay(layoutEditorView.btn_editToggle.toggleValue);
			#endif
			}
			popScroll();
			
			layoutEditorView.drawOverlay();
			
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
	
	liveUi.shut();
	
	framework.shutdown();
	
	return 0;
}
