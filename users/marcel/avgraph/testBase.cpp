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

#include "Ease.h"
#include "framework.h"
#include "testBase.h"
#include <map>

extern const int GFX_SX;
extern const int GFX_SY;

extern SDL_Cursor * handCursor;

enum ButtonResult
{
	kButtonIdle,
	kButtonHover,
	kButtonPress
};

struct ButtonState
{
	float fadeTimer = .2f;
	float fadeTimerRcp = 1.f / fadeTimer;
	
	bool hover = false;
};

struct TestState
{
	bool firstFrame;
	
	float uiTimer;
	float uiTimerRcp;
	
	float scrollTimer;
	float scrollTimerRcp;
	
	std::map<std::string, ButtonState> buttonState;
	
	bool tickMenu;
	bool drawMenu;

	int drawX;

	bool isHovering;
	
	std::string instructions;
	std::string about;
	
	TestState()
		: firstFrame(true)
		, uiTimer(0.f)
		, uiTimerRcp(0.f)
		, scrollTimer(0.f)
		, scrollTimerRcp(0.f)
		, buttonState()
		, tickMenu(false)
		, drawMenu(false)
		, drawX(0)
		, isHovering(false)
		, instructions()
		, about()
	{
		uiTimer = 1.5f;
		uiTimerRcp = 1.f / uiTimer;
		scrollTimer = .3f;
		scrollTimerRcp = 1.f / scrollTimer;
	}
};

static TestState s_testState;

void beginTest(TestFunction t)
{
	s_testState = TestState();
}

void endTest(TestFunction t)
{
}

static ButtonResult doButton(const int _x, const int _y, const int alignX, const int alignY, const char * text)
{
	ButtonState & buttonState = s_testState.buttonState[text];
	buttonState.fadeTimer = std::max(0.f, buttonState.fadeTimer - framework.timeStep);
	
	bool clicked = false;
	
	const float _fade = buttonState.fadeTimer * buttonState.fadeTimerRcp;
	const float fade = _fade < .8f ? _fade / .8f : 1.f;
	
	const int opacity = lerp(200, 255, fade);
	const float scale = lerp(.9f, 1.f, fade);
	const float cornerRadius = 10.f;
	const float strokeSize = 2.f;
	const int fontSize = 20;
	
	float sx;
	float sy;
	measureText(fontSize, sx, sy, "%s", text);
	
	sx += 30;
	sy += 20;
	
	const float x = _x + sx/2 * alignX;
	const float y = _y + sy/2 * alignY;
	
	if (s_testState.tickMenu)
	{
		bool hover = false;
		
		if (mouse.x >= x-sx/2 && mouse.y >= y-sy/2 && mouse.x <= x + sx/2 && mouse.y <= y + sy/2)
		{
			hover = true;
			
			buttonState.fadeTimer = 1.f / buttonState.fadeTimerRcp;
		}
		
		if (hover != buttonState.hover)
		{
			buttonState.hover = hover;
			
			Sound("menuselect.ogg").play();
		}
		
		if (hover && mouse.wentDown(BUTTON_LEFT))
		{
			clicked = true;
		}
		
		s_testState.isHovering |= hover;
	}
	
	if (s_testState.drawMenu)
	{
		gxPushMatrix();
		{
			gxTranslatef(x, y, 0);
			gxScalef(scale, scale, 1);
			
			hqBegin(HQ_FILLED_ROUNDED_RECTS);
			{
				setColor(0, 0, 100, opacity);
				hqFillRoundedRect(-sx/2 - strokeSize, -sy/2 - strokeSize, +sx/2 + strokeSize, +sy/2 + strokeSize, cornerRadius + strokeSize);
			}
			hqEnd();
			
			hqBegin(HQ_FILLED_ROUNDED_RECTS);
			{
				setColor(255, 255, 255, opacity);
				hqFillRoundedRect(-sx/2, -sy/2, +sx/2, +sy/2, cornerRadius);
			}
			hqEnd();
			
			setColor(0, 0, 100, opacity);
			drawText(0, 0, fontSize, 0, 0, text);
		}
		gxPopMatrix();
	}
	
	s_testState.drawX += sx * alignX;
	
	if (clicked)
		return kButtonPress;
	else if (buttonState.hover)
		return kButtonHover;
	else
		return kButtonIdle;
}

static void drawTextbox(const int x, const int y, const std::string & text)
{
	const int maxSx = 300;
	const float fontSize = 16.f;
	const int padding = 2;
	const int radius = 6;
	const int margin = 4;
	
	float sx;
	float sy;
	measureTextArea(fontSize, maxSx, sx, sy, "%s", text.c_str());
	
	
	
	const float x1 = x - maxSx;
	const float y1 = y - sy;
	const float x2 = x;
	const float y2 = y;
	
	hqBegin(HQ_FILLED_ROUNDED_RECTS);
	{
		setColor(0, 0, 0);
		hqFillRoundedRect(
			x1 - radius - margin - padding,
			y1 - radius - margin - padding,
			x2 + radius + margin + padding,
			y2 + radius + margin + padding,
			radius + padding);
		
		setColor(255, 255, 200);
		hqFillRoundedRect(
			x1 - radius - margin,
			y1 - radius - margin,
			x2 + radius + margin,
			y2 + radius + margin,
			radius);
	}
	hqEnd();
	
	setColor(colorBlack);
	drawTextArea(x1, y1, maxSx, sy, fontSize, 0, 0, "%s", text.c_str());
}

static bool doMenu(const bool tickMenu, const bool drawMenu)
{
	bool result = true;
	
	s_testState.tickMenu = tickMenu;
	s_testState.drawMenu = drawMenu;
	
	s_testState.drawX = 25;
	if (doButton(s_testState.drawX, GFX_SY - 25, +1, -1, "Back") == kButtonPress)
	{
		result = false;
	}
	
	if (s_testState.about.empty() == false)
	{
		s_testState.drawX = GFX_SX - 25;
		if (doButton(s_testState.drawX, GFX_SY - 25, -1, -1, "About") == kButtonHover)
		{
			drawTextbox(mouse.x, mouse.y, s_testState.about);
		}
	}
	
	if (s_testState.instructions.empty() == false)
	{
		s_testState.drawX -= 10;
		if (doButton(s_testState.drawX, GFX_SY - 25, -1, -1, "Instructions") == kButtonHover)
		{
			if (s_testState.drawMenu)
			{
				drawTextbox(mouse.x, mouse.y, s_testState.instructions);
			}
		}
	}
	
	return result;
}

void drawTestUi()
{
	TestState & s = s_testState;
	
	s_testState.isHovering = false;
	
	pushFontMode(FONT_SDF);
	setFont("calibri.ttf");
	
	if (s.firstFrame)
	{
		// test initialization may take a while, so ignore the tick the first time this function is called, to
		// avoid a large framework.timeStep from immediately ending the animation
		
		s.firstFrame = false;
	}
	else
	{
		const float dt = framework.timeStep;
		
		s.uiTimer = std::max(0.f, s.uiTimer - dt);
		s.scrollTimer = std::max(0.f, s.scrollTimer - dt);
		
		if (mouse.y < 50)
			s.uiTimer = 1.f / s.uiTimerRcp;
	}
	
	const float scrollTime = s.scrollTimer * s.scrollTimerRcp;
	const float scroll = EvalEase(scrollTime, kEaseType_SineIn, 0.f);
	
	gxPushMatrix();
	{
		if (scroll > 0.f)
		{
			setColorf(1.f, 1.f, 1.f, scroll);
			drawRect(0, GFX_SY, GFX_SX, 0);
		}
		
	#if 1
		doMenu(false, true);
	#else
		const int alpha = offset * 255;
		const float scale = offset;
		const float cornerRadius = 10.f;
		const float strokeSize = 2.f;
		
		const int sx = 160;
		const int sy = 40;
		
		gxTranslatef(GFX_SX - sx/2 - 25, GFX_SY - sy/2 - 25, 0);
		gxScalef(scale, scale, 1);
		
		hqBegin(HQ_FILLED_ROUNDED_RECTS);
		{
			setColor(0, 0, 100, alpha);
			hqFillRoundedRect(-sx/2 - strokeSize, -sy/2 - strokeSize, +sx/2 + strokeSize, +sy/2 + strokeSize, cornerRadius + strokeSize);
		}
		hqEnd();
		
		hqBegin(HQ_FILLED_ROUNDED_RECTS);
		{
			setColor(255, 255, 255, alpha);
			hqFillRoundedRect(-sx/2, -sy/2, +sx/2, +sy/2, cornerRadius);
		}
		hqEnd();
		
		setColor(0, 0, 100, alpha);
		drawText(0, 0, 20, 0, 0, "Help");
	#endif
	}
	gxPopMatrix();
	
	popFontMode();
}

bool tickTestUi()
{
	bool result = true;
	
	result &= doMenu(true, false);
	
	result &= keyboard.wentDown(SDLK_SPACE) == false;
	
	if (s_testState.isHovering)
		SDL_SetCursor(handCursor);
	else
		SDL_SetCursor(SDL_GetDefaultCursor());
	
	return result;
}

void setInstructions(const char * instructions)
{
	s_testState.instructions = instructions;
}

void setAbout(const char * about)
{
	s_testState.about = about;
}
