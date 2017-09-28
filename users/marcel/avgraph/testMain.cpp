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

#include "framework.h"
#include "testBase.h"
#include <map>
#include <string>

extern const int GFX_SX;
extern const int GFX_SY;

extern void testDatGui();
extern void testNanovg();
extern void testFourier1d();
extern void testFourier2d();
extern void testChaosGame();
extern void testImpulseResponseMeasurement();
extern void testTextureAtlas();
extern void testDynamicTextureAtlas();
extern void testDotDetector();
extern void testDotTracker();
extern void testAudiochannels();
extern void testThreading();
extern void testStbTruetype();
extern void testMsdfgen();
#ifdef MACOS
extern void testDeepbelief();
#endif
extern void testImageCpuDelayLine();
#ifndef WIN32
extern void testXmm();
#endif
extern void testHqPrimitives();
extern void testHrtf();

extern void testMain();
extern void testMenu();

//

#define NUM_LINKS 5

struct ButtonState;

static bool hasMouseHover = false;
SDL_Cursor * handCursor = nullptr;

struct ButtonLink
{
	ButtonState * b = nullptr;
	float d;
	float a;
};

struct ButtonState
{
	float x = 0.f;
	float y = 0.f;
	float r = 50.f;
	float sx = 200.f;
	float sy = 100.f;
	
	float speedX = 0.f;
	float speedY = 0.f;
	float ax = 0.f;
	float ay = 0.f;
	
	std::string caption;
	std::string text;
	
	bool isBack = false;
	
	bool hover = false;
	bool isDown = false;
	
	float hoverAnim = 0.f;
	
	ButtonLink links[NUM_LINKS];
	
	ButtonState()
	{
	}
	
	ButtonState(const char * _caption, const char * _text)
	{
		x = random(0, GFX_SX);
		y = random(0, GFX_SY);
		
		caption = _caption;
		text = _text;
	}
	
	bool tick(const float dt)
	{
		const float mx = mouse.x - x;
		const float my = mouse.y - y;
		
		bool isInside;
		
		if (hover)
		{
			isInside =
				mx > -sx/2 &&
				mx < +sx/2 &&
				my > -sy/2 &&
				my < +sy/2;
		}
		else
		{
			isInside = std::hypotf(mx, my) < r;
		}
		
		if (hover)
		{
			hoverAnim = std::min(hoverAnim + dt / .3f, 1.f);
		}
		else
		{
			hoverAnim = std::max(hoverAnim - dt / .15f, 0.f);
		}
		
		bool clicked = false;
		
		if (isInside)
		{
			if (hover == false)
				Sound("menuselect.ogg").play();
			
			hover = true;
		}
		else
		{
			hover = false;
		}
		
		if (mouse.wentDown(BUTTON_LEFT))
		{
			if (hover)
			{
				isDown = true;
			}
		}
		
		if (mouse.wentUp(BUTTON_LEFT))
		{
			if (isDown)
			{
				isDown = false;
				
				if (hover)
				{
					clicked = true;
				}
			}
		}
		
		if (clicked)
			Sound("menuselect.ogg").play();
		
		hasMouseHover |= hover;
		
		return clicked;
	}
	
	void draw() const
	{
		gxPushMatrix();
		{
			gxTranslatef(x, y, 0);
			
			Color color;
			
			if (hover && isDown)
				color = Color(100, 100, 200);
			else if (hover)
				color = Color(255, 255, 255);
			else if (isBack)
				color = Color(255, 200, 200);
			else
				color = Color(200, 200, 255);
			
			gxPushMatrix();
			{
				gxScalef(1.f - hoverAnim, 1.f - hoverAnim, 1);
				
				hqBegin(HQ_FILLED_CIRCLES);
				{
					setColor(color);
					hqFillCircle(0, 0, r);
				}
				hqEnd();
				
				setColor(0, 0, 100);
				drawText(0, 0, 32, 0, 0, "%s", caption.c_str());
				
				hqBegin(HQ_STROKED_CIRCLES);
				{
					setColor(0, 0, 100);
					hqStrokeCircle(0, 0, r, 2.5f);
				}
				hqEnd();
			}
			gxPopMatrix();
			
			gxPushMatrix();
			{
				gxScalef(hoverAnim, hoverAnim, 1);
				
				hqBegin(HQ_FILLED_ROUNDED_RECTS);
				{
					const int border = 2;
					
					setColor(0, 0, 100);
					hqFillRoundedRect(-sx/2 - border, -sy/2 - border, sx/2 + border, sy/2 + border, 10 + border);
					
					setColor(color);
					hqFillRoundedRect(-sx/2, -sy/2, sx/2, sy/2, 10);
				}
				hqEnd();
				
				setColor(0, 0, 100);
				drawText(0, 0, 20, 0, 0, "%s", text.c_str());
			}
			gxPopMatrix();
		}
		gxPopMatrix();
	}
};

static std::map<std::string, ButtonState> buttonStates;

static bool menuTick;
static bool menuDraw;
static float menuDt;
static bool buttonPressed = false;

static bool doButton(const char * caption, const char * name, TestFunction testFunction, const bool isBack = false)
{
	bool clicked = false;
	
	auto i = buttonStates.find(name);
	
	if (i == buttonStates.end())
	{
		ButtonState state(caption, name);
		
		state.isBack = isBack;
		
		i = buttonStates.insert(std::make_pair(name, state)).first;
	}
	
	ButtonState & state = i->second;
	
	if (menuTick)
	{
		clicked = state.tick(menuDt);
	}
	
	if (menuDraw)
	{
		setFont("calibri.ttf");
		pushFontMode(FONT_SDF);
		{
			state.draw();
		}
		popFontMode();
	}
	
	buttonPressed |= clicked;
	
	if (clicked && testFunction != nullptr)
	{
		SDL_SetCursor(SDL_GetDefaultCursor());
		
		beginTest(testFunction);
		
		testFunction();
		
		endTest(testFunction);
	}
	
	return clicked;
}

static void addSpringForce(
	const float x1,
	const float y1,
	const float x2,
	const float y2,
	const float targetDistance,
	const float springConstant,
	float & ax, float & ay)
{
	const float dx = x1 - x2;
	const float dy = y1 - y2;
	
	const float ds = std::hypotf(dx, dy) + .1f;
	const float dd = ds - targetDistance;
	
	const float f = - dd * springConstant;
	const float a = f;
	
	ax += (dx / ds) * a;
	ay += (dy / ds) * a;
}

static bool doMenus(const bool tick, const bool draw, const float dt)
{
	menuTick = tick;
	menuDraw = draw;
	menuDt = dt;
	
	if (tick)
	{
		for (auto & bi : buttonStates)
		{
			auto & b = bi.second;
			
			std::vector<ButtonLink> links;
			
			for (auto & oi : buttonStates)
			{
				auto & o = oi.second;
				
				if (&o == &b)
					continue;
				
				const float dx = o.x - b.x;
				const float dy = o.y - b.y;
				const float ds = std::sqrtf(dx * dx + dy * dy);
				
				ButtonLink bl;
				bl.b = &o;
				bl.d = ds;
				
				links.push_back(bl);
			}
			
			std::sort(links.begin(), links.end(), [](const ButtonLink & bl1, const ButtonLink & bl2) { return bl1.d < bl2.d; });
			
			for (int i = 0; i < NUM_LINKS; ++i)
				b.links[i] = ButtonLink();
			for (int i = 0; i < NUM_LINKS && i < links.size(); ++i)
				b.links[i] = links[i];
		}
		
		for (auto & bi : buttonStates)
		{
			auto & b = bi.second;
			
			float ax = 0.f;
			float ay = 0.f;
			
			addSpringForce(b.x, b.y, GFX_SX/2, GFX_SY/2, 0.f, 1.f / 10.f, ax, ay);
			
			for (int i = 0; i < NUM_LINKS; ++i)
			{
				auto * o = b.links[i].b;
				
				if (o == nullptr)
					continue;
				
				addSpringForce(b.x, b.y, b.links[i].b->x, b.links[i].b->y, o->hover ? 300.f : 200.f, 1.f / 1.f, ax, ay);
			}
			
			b.ax = ax;
			b.ay = ay;
		}
		
		const float falloff = std::powf(.2f, dt);
		
		for (auto & bi : buttonStates)
		{
			auto & b = bi.second;
			
			b.speedX += b.ax * dt;
			b.speedY += b.ay * dt;
			
			b.speedX *= falloff;
			b.speedY *= falloff;
			
			b.x += b.speedX * dt;
			b.y += b.speedY * dt;
		}
	}
	
	if (draw)
	{
		setColor(200, 200, 255);
		
		hqBegin(HQ_LINES);
		{
			for (auto & bi : buttonStates)
			{
				auto & b = bi.second;
				
				for (int i = 0; i < NUM_LINKS; ++i)
				{
					if (b.links[i].b == nullptr)
						continue;
					
					hqLine(b.x, b.y, 3.f, b.links[i].b->x, b.links[i].b->y, 0.f);
				}
			}
		}
		hqEnd();
	}
	
	if (menuTick)
	{
		hasMouseHover = false;
		buttonPressed = false;
	}
	
	doButton("KOAS", "Chaos Game", testChaosGame);
	doButton("DaGu", "DatGUI", testDatGui);
#ifdef MACOS
	doButton("DdBe", "Deep Belief SDK", testDeepbelief);
#endif
	doButton("DtDt", "Dot Detector", testDotDetector);
	doButton("DtTr", "Dot Tracker", testDotTracker);
	doButton("DTAtl", "Dynamic Texture Atlas", testDynamicTextureAtlas);
	//doButton("Fr1D", "1D Fourier Analysis", testFourier1d);
	doButton("Fr2D", "2D Fourier Analysis", testFourier2d);
	doButton("ImDL", "CPU-image delay line", testImageCpuDelayLine);
	doButton("DrPr", "Drawing Primitives", testHqPrimitives);
	doButton("HRTF", "Binaural Sound", testHrtf);
	doButton("IRm", "Impulse-Response", testImpulseResponseMeasurement);
	doButton("MSDF", "MSDFGEN", testMsdfgen);
	doButton("NVg", "NanoVG", testNanovg);
	//doButton("TT", "STB TrueType", testStbTruetype);
	//doButton("TAtl", "Texture Atlas", testTextureAtlas);
	//doButton("Thr", "Threading", testThreading);
#ifndef WIN32
	doButton("XMM", "XMM Gesture Follower", testXmm);
#endif

	const bool result = doButton("BACK", "Back", nullptr, true);
	
	if (menuTick)
	{
		if (hasMouseHover)
			SDL_SetCursor(handCursor);
		else
			SDL_SetCursor(SDL_GetDefaultCursor());
	}
	
	return result;
}

struct MainButton
{
	std::string caption;
	std::string image;
	
	float currentX;
	float currentY;
	
	float desiredX;
	float desiredY;
	
	float sx;
	float sy;
	
	bool hover;
	bool isDown;
	bool clicked;
	
	MainButton()
		: caption()
		, image()
		, currentX(0.f)
		, currentY(0.f)
		, desiredX(100.f)
		, desiredY(100.f)
		, sx(500.f)
		, sy(200.f)
		, hover(false)
		, isDown(false)
		, clicked(false)
	{
	}
	
	bool process(const bool tick, const bool draw, const float dt)
	{
		clicked = false;
		
		if (tick)
		{
			const float retainPerSecond = .2f;
			const float retainThisFrame = std::powf(retainPerSecond, dt);
			
			currentX = lerp(desiredX, currentX, retainThisFrame);
			currentY = lerp(desiredY, currentY, retainThisFrame);
			
			const bool isInside =
				mouse.x > currentX &&
				mouse.y > currentY &&
				mouse.x < currentX + sx &&
				mouse.y < currentY + sy;
			
			if (isInside)
			{
				if (hover == false)
				{
					hover = true;
					
					Sound("menuselect.ogg").play();
				}
			}
			else
			{
				if (hover == true)
				{
					hover = false;
					
					Sound("menuselect.ogg").play();
				}
			}
				
			if (hover)
			{
				if (mouse.wentDown(BUTTON_LEFT))
					isDown = true;
			}
			
			if (isDown)
			{
				if (mouse.wentUp(BUTTON_LEFT))
				{
					isDown = false;
					
					if (hover)
					{
						clicked = true;
						
						Sound("menuselect.ogg").play();
					}
				}
			}
			
			hasMouseHover |= hover;
		}
		
		if (draw)
		{
			gxPushMatrix();
			gxTranslatef(currentX, currentY, 0);
			
			if (hover)
			{
				hqBegin(HQ_FILLED_ROUNDED_RECTS);
				setColor(colorWhite);
				const float border = 10.f;
				hqFillRoundedRect(-border, -border, sx+border, sy+border, 20+border);
				hqEnd();
			}
			
			hqBegin(HQ_FILLED_ROUNDED_RECTS);
			if (isDown)
				setColor(100, 0, 0);
			else if (hover)
				setColor(200, 100, 100);
			else
				setColor(200, 0, 0);
			hqFillRoundedRect(0, 0, sx, sy, 20);
			hqEnd();
			
			setFont("calibri.ttf");
			pushFontMode(FONT_SDF);
			{
				setColor(colorWhite);
				drawText(sx/2, sy/2, 48, 0, 0, "%s", caption.c_str());
			}
			popFontMode();
			
			gxPopMatrix();
		}
		
		return clicked;
	}
};

struct MainButtons
{
	MainButton avgraphButton;
	MainButton testsButton;
	
	MainButtons()
		: avgraphButton()
		, testsButton()
	{
		avgraphButton.caption = "A/V Graph Editor";
		avgraphButton.currentX = 100.f;
		avgraphButton.currentY = 200.f;
		avgraphButton.desiredX = (GFX_SX - avgraphButton.sx) / 2.f;
		avgraphButton.desiredY = avgraphButton.currentY;
		
		testsButton.caption = "Tests Portal";
		testsButton.currentX = GFX_SX - testsButton.sx - 100.f;
		testsButton.currentY = 450.f;
		testsButton.desiredX = (GFX_SX - testsButton.sx) / 2.f;
		testsButton.desiredY = testsButton.currentY;
	}
	
	bool process(const bool tick, const bool draw, const float dt)
	{
		bool result = false;
		
		if (tick)
		{
			hasMouseHover = false;
			buttonPressed = false;
			
			if (keyboard.wentDown(SDLK_ESCAPE))
				result = true;
		}
		
		if (avgraphButton.process(tick, draw, dt))
		{
			result = true;
		}
		
		if (testsButton.process(tick, draw, dt))
		{
			testMenu();
			
			framework.process();
		}
		
		if (draw)
		{
			setFont("calibri.ttf");
			setColor(100, 100, 130);
			drawText(GFX_SX/2, 100, 48, 0, 0, "Where do you want to go today?");
		}
		
		if (tick)
		{
			if (hasMouseHover)
				SDL_SetCursor(handCursor);
			else
				SDL_SetCursor(SDL_GetDefaultCursor());
		}
		
		return result;
	}
};

static MainButtons mainButtons;

static bool doMainButtons(const bool tick, const bool draw, const float dt)
{
	return mainButtons.process(tick, draw, dt);
}

void testMain()
{
	beginTest(testMain);
	
	handCursor = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_HAND);
	
	Surface surface(GFX_SX, GFX_SY, true);
	
	bool stop = false;
	
	double blurStrength = 0.f;
	
	do
	{
		framework.process();

		const float dt = framework.timeStep;

		//

		stop = doMainButtons(true, false, dt) || keyboard.wentDown(SDLK_ESCAPE);

		if (buttonPressed)
		{
			mainButtons = MainButtons();
			
			blurStrength = 1.0;
		}
		
		blurStrength *= std::pow(.01, double(dt * 4.0));
		
		//

		framework.beginDraw(0, 0, 0, 0);
		{
			pushSurface(&surface);
			{
				surface.clear(230, 230, 230, 0);
				
				doMainButtons(false, true, dt);
			}
			popSurface();
			
			//surface.invert();
			surface.gaussianBlur(blurStrength * 100.f, blurStrength * 100.f, std::ceilf(blurStrength * 100.f));
			
			setColor(colorWhite);
			surface.blit(BLEND_OPAQUE);
		}
		framework.endDraw();
	} while (stop == false);
	
	SDL_FreeCursor(handCursor);
	handCursor = nullptr;
}

void testMenu()
{
	beginTest(testMenu);
	
	Surface surface(GFX_SX, GFX_SY, true);
	
	bool stop = false;
	
	double blurStrength = 0.f;
	
	do
	{
		framework.process();

		const float dt = framework.timeStep;

		//

		stop = doMenus(true, false, dt) || keyboard.wentDown(SDLK_ESCAPE);

		if (buttonPressed)
			blurStrength = 1.0;
		
		blurStrength *= std::pow(.01, double(dt * 4.0));
		
		//

		framework.beginDraw(0, 0, 0, 0);
		{
			pushSurface(&surface);
			{
				surface.clear(230, 230, 230, 0);
				
				doMenus(false, true, dt);
			}
			popSurface();
			
			//surface.invert();
			surface.gaussianBlur(blurStrength * 100.f, blurStrength * 100.f, std::ceilf(blurStrength * 100.f));
			
			setColor(colorWhite);
			surface.blit(BLEND_OPAQUE);
		}
		framework.endDraw();
	} while (stop == false);
}
