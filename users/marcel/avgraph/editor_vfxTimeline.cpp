/*
	Copyright (C) 2017 Marcel Smit
	marcel303@gmail.com
	https://www.facebook.com/marcel.smit981

	Permission is hereby granted, free of charge, to any person
	obtaining a copy of this software and associated documentation
	files (the "Software"), to deal in the Software without
	restriction, including without limitation the rights to use,
	copy, modify, merge, publish, distribute, sublicense, and/or
	sell copies of the Software, and to permit persons to whom the
	Software is furnished to do so, subject to the following
	conditions:

	The above copyright notice and this permission notice shall be
	included in all copies or substantial portions of the Software.

	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
	EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
	OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
	NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
	HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
	WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
	FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
	OTHER DEALINGS IN THE SOFTWARE.
*/

#include "editor_vfxTimeline.h"
#include "framework.h"
#include "tinyxml2.h"
#include "vfxGraph.h"
#include "vfxTypes.h"
#include "../libparticle/ui.h" // todo : remove

//

using namespace tinyxml2;

//

static void doVfxTimeline(VfxTimeline & timeline, int & selectedKeyIndex, const char * name, const float dt);

//

Editor_VfxTimeline::Editor_VfxTimeline()
	: GraphEdit_NodeEditorBase()
	, uiState(nullptr)
	, timeline(nullptr)
	, selectedKeyIndex(-1)
{
	uiState = new UiState();
	uiState->sx = GFX_SX - 100;
}

Editor_VfxTimeline::~Editor_VfxTimeline()
{
	freeVfxNodeResource(timeline);
	
	delete uiState;
	uiState = nullptr;
}

void Editor_VfxTimeline::getSize(int & sx, int & sy) const
{
	sx = uiState->sx;
	sy = 200;
}

void Editor_VfxTimeline::setPosition(const int x, const int y)
{
	uiState->x = x;
	uiState->y = y;
}
	
bool Editor_VfxTimeline::tick(const float dt, const bool inputIsCaptured)
{
	makeActive(uiState, true, false);

	if (timeline != nullptr)
	{
		doVfxTimeline(*timeline, selectedKeyIndex, "timeline", dt);
	}
	
	return uiState->activeElem != nullptr;
}

void Editor_VfxTimeline::draw() const
{
	makeActive(uiState, false, true);

	if (timeline != nullptr)
	{
		doVfxTimeline(*timeline, selectedKeyIndex, "timeline", 0.f);
	}
}
	
void Editor_VfxTimeline::setResource(const GraphNode & node, const char * type, const char * name, const char * text)
{
	Assert(timeline == nullptr);
	
	if (createVfxNodeResource<VfxTimeline>(node, type, name, timeline))
	{
		if (text != nullptr)
		{
			XMLDocument d;
			
			if (d.Parse(text) == tinyxml2::XML_SUCCESS)
			{
				tinyxml2::XMLElement * e = d.FirstChildElement();
				
				if (e != nullptr)
				{
					timeline->load(e);
				}
			}
		}
	}
}

bool Editor_VfxTimeline::serializeResource(std::string & text) const
{
	if (timeline != nullptr)
	{
		tinyxml2::XMLPrinter p;
		p.OpenElement("value");
		{
			timeline->save(&p);
		}
		p.CloseElement();
		
		text = p.CStr();
		
		return true;
	}
	else
	{
		return false;
	}
}

//

static float screenToBeat(const int x1, const int x2, const int x, const float offset, const float bpm, const float length)
{
	const float numBeats = length * bpm / 60.f;
	
	return saturate((x - x1) / float(x2 - x1)) * numBeats + offset;
}

static float beatToScreen(int x1, int x2, float beat, const float bpm, const float length)
{
	const float numBeats = length * bpm / 60.f;
	
	return x1 + (x2 - x1) * beat / numBeats;
}

static VfxTimeline::Key * findNearestKey(VfxTimeline & timeline, const int x1, const int x2, const float beat, const float maxDeviation)
{
	VfxTimeline::Key * nearestKey = 0;
	float nearestDistance = 0.f;

	for (int i = 0; i < timeline.numKeys; ++i)
	{
		const float dt = beatToScreen(x1, x2, timeline.keys[i].beat, timeline.bpm, timeline.length) - beatToScreen(x1, x2, beat, timeline.bpm, timeline.length);
		
		const float distance = std::sqrtf(dt * dt);

		if (distance < maxDeviation && (distance < nearestDistance || nearestKey == 0))
		{
			nearestKey = &timeline.keys[i];
			nearestDistance = distance;
		}
	}

	return nearestKey;
}

static void resetSelectedKeyUi()
{
	g_menu->getElem("beat").reset();
	g_menu->getElem("id").reset();
}

static void doVfxTimeline(VfxTimeline & timeline, int & selectedKeyIndex, const char * name, const float dt)
{
	const int kTimelineHeight = 20;
	const Color kBackgroundFocusColor(0.f, 0.f, 1.f, g_uiState->opacity * .7f);
	const Color kBackgroundColor(0.f, 0.f, 0.f, g_uiState->opacity * .8f);
	
	pushMenu(name);
	
	doBreak();
	
	UiElem & elem = g_menu->getElem("timeline");
	
	enum Vars
	{
		kVar_SelectedKey,
		kVar_IsDragging,
		kVar_DragOffset
	};
	
	VfxTimeline::Key *& selectedKey = elem.getPointer<VfxTimeline::Key*>(kVar_SelectedKey, nullptr);
	bool & isDragging = elem.getBool(kVar_IsDragging, false);
	float & dragOffset = elem.getFloat(kVar_DragOffset, 0.f);
	
	if (selectedKeyIndex >= 0 && selectedKeyIndex < timeline.numKeys)
		selectedKey = &timeline.keys[selectedKeyIndex];
	
	const float kMaxSelectionDeviation = 5;
	
	const int x1 = g_drawX;
	const int x2 = g_drawX + g_menu->sx;
	const int y1 = g_drawY;
	const int y2 = g_drawY + kTimelineHeight;

	g_drawY += kTimelineHeight;

	const int cx1 = x1;
	const int cy1 = y1;
	const int cx2 = x2;
	const int cy2 = y2;

	if (g_doActions)
	{
		elem.tick(x1, y1, x2, y2);

		if (!elem.isActive && false)
			selectedKey = nullptr;
		else if (elem.hasFocus)
		{
			if (mouse.wentDown(BUTTON_LEFT))
			{
				// select or insert key

				const float beat = screenToBeat(x1, x2, mouse.x, 0.f, timeline.bpm, timeline.length);
				auto key = findNearestKey(timeline, x1, x2, beat, kMaxSelectionDeviation);
				
				if (key == nullptr)
				{
					// insert a new key

					if (timeline.allocKey(key))
					{
						key->beat = beat;
						key->id = 0;

						key = timeline.sortKeys(key);
					}
				}
				
				selectedKey = key;

				if (selectedKey != nullptr)
				{
					isDragging = true;
					dragOffset = key->beat - beat;
				}
				
				resetSelectedKeyUi();
			}
			
			if (!mouse.isDown(BUTTON_LEFT))
			{
				if (mouse.wentDown(BUTTON_RIGHT) || keyboard.wentDown(SDLK_DELETE) || keyboard.wentDown(SDLK_BACKSPACE))
				{
					selectedKey = nullptr;

					// erase key

					const float beat = screenToBeat(x1, x2, mouse.x, 0.f, timeline.bpm, timeline.length);
					auto * key = findNearestKey(timeline, x1, x2, beat, kMaxSelectionDeviation);

					if (key)
					{
						timeline.freeKey(key);
					}
					
					g_menu->getElem("beat").deactivate();
				}
			}
		}

		if (isDragging)
		{
			if (elem.isActive && selectedKey != nullptr && mouse.isDown(BUTTON_LEFT))
			{
				// move selected key around

				const float beat = screenToBeat(x1, x2, mouse.x, dragOffset, timeline.bpm, timeline.length);

				selectedKey->beat = beat;
				selectedKey = timeline.sortKeys(selectedKey);

				fassert(selectedKey->beat == beat);
				
				resetSelectedKeyUi();
			}
			else
			{
				float beat = selectedKey->beat;
				
				if (!keyboard.isDown(SDLK_LSHIFT) && !keyboard.isDown(SDLK_RSHIFT))
					beat = std::round(beat * 4.f) / 4.f;
				
				selectedKey->beat = beat;
				selectedKey = timeline.sortKeys(selectedKey);

				fassert(selectedKey->beat == beat);
				
				resetSelectedKeyUi();
				
				isDragging = false;
			}
		}
		else if (elem.isActive && selectedKey != nullptr)
		{
			if (keyboard.wentDown(SDLK_LEFT, true))
			{
				selectedKey->beat -= 1.f / 4.f;
				if (!keyboard.isDown(SDLK_LSHIFT) && !keyboard.isDown(SDLK_RSHIFT))
					selectedKey->beat = std::round(selectedKey->beat * 4.f) / 4.f;
				selectedKey = timeline.sortKeys(selectedKey);
				resetSelectedKeyUi();
			}
			
			if (keyboard.wentDown(SDLK_RIGHT, true))
			{
				selectedKey->beat += 1.f / 4.f;
				if (!keyboard.isDown(SDLK_LSHIFT) && !keyboard.isDown(SDLK_RSHIFT))
					selectedKey->beat = std::round(selectedKey->beat * 4.f) / 4.f;
				selectedKey = timeline.sortKeys(selectedKey);
				resetSelectedKeyUi();
			}
			
			if (keyboard.wentDown(SDLK_UP, true))
			{
				selectedKey->id++;
				resetSelectedKeyUi();
			}
			
			if (keyboard.wentDown(SDLK_DOWN, true))
			{
				selectedKey->id--;
				resetSelectedKeyUi();
			}
		}
	}

	if (g_doDraw)
	{
		if (elem.hasFocus)
			setColor(kBackgroundFocusColor);
		else
			setColor(kBackgroundColor);
		drawRect(x1, y1, x2, y2);
		setColor(colorBlue);
		drawRectLine(x1, y1, x2, y2);

		setColor(colorWhite);
		drawUiRectCheckered(cx1, cy1, cx2, cy2, 4.f);
		
		const int numBeatMarkers = int(timeline.length * timeline.bpm / 60.f);
		
		hqBegin(HQ_LINES);
		{
			for (int i = 1; i < numBeatMarkers * 4; ++i)
			{
				if ((i % 4) == 0)
					continue;
				
				const float x = beatToScreen(x1, x2, i / 4.f, timeline.bpm, timeline.length);
				
				setColor(127, 127, 127, 127);
				hqLine(x, y1, 1.2f, x, y2, 1.2f);
			}
			
			for (int i = 1; i < numBeatMarkers; ++i)
			{
				const float x = beatToScreen(x1, x2, i, timeline.bpm, timeline.length);
				
				setColor(255, 127, 0, 127);
				hqLine(x, y1, 1.2f, x, y2, 1.2f);
			}
		}
		hqEnd();
		
		{
			const float beat = screenToBeat(x1, x2, mouse.x, 0.f, timeline.bpm, timeline.length);
			auto key = findNearestKey(timeline, x1, x2, beat, kMaxSelectionDeviation);
			for (int i = 0; i < timeline.numKeys; ++i)
			{
				const float c =
					  !elem.hasFocus ? .5f
					: (key == &timeline.keys[i]) ? 1.f
					: (selectedKey == &timeline.keys[i]) ? .8f
					: .5f;
				const float x = beatToScreen(x1, x2, timeline.keys[i].beat, timeline.bpm, timeline.length);
				const float y = (y1 + y2) / 2.f;
				drawUiCircle(x, y, 5.5f, c, c, c, 1.f);
			}
		}
	}
	
	doTextBox(timeline.length, "length (sec)", 0.f, .3f, false, dt);
	doTextBox(timeline.bpm, "bpm", .3f, .3f, true, dt);
	
	if (selectedKey != nullptr)
	{
		doTextBox(selectedKey->beat, "beat", 0.f, .3f, false, dt);
		doTextBox(selectedKey->id, "id", .3f, .3f, true, dt);
		selectedKey = timeline.sortKeys(selectedKey);
		fassert(selectedKey != nullptr);
	}
	
	doBreak();
	
	selectedKeyIndex = selectedKey == nullptr ? -1 : selectedKey - timeline.keys;
	
	popMenu();
}
