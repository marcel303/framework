#include "framework.h"

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
extern void testDeepbelief();
extern void testImageCpuDelayLine();
extern void testXmm();
extern void testHqPrimitives();

//

struct ButtonState
{
	float x = 0.f;
	float y = 0.f;
	float sx = 200.f;
	float sy = 100.f;
	
	float ax = 0.f;
	float ay = 0.f;
	
	std::string text;
	
	bool hover = false;
	bool isDown = false;
	
	ButtonState()
	{
	}
	
	ButtonState(const char * _text)
	{
		x = random(0, GFX_SX);
		y = random(0, GFX_SY);
		
		text = _text;
	}
	
	bool tick(const float dt)
	{
		const float mx = mouse.x - x;
		const float my = mouse.y - y;
		
		const bool isInside =
			mx >= 0.f &&
			mx < sx &&
			my >= 0.f &&
			my < sy;
		
		bool clicked = false;
		
		if (isInside)
		{
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
		
		return clicked;
	}
	
	void draw() const
	{
		gxPushMatrix();
		{
			gxTranslatef(x, y, 0);
			
			hqBegin(HQ_FILLED_ROUNDED_RECTS);
			{
				if (hover && isDown)
					setColor(100, 100, 200);
				else if (hover)
					setColor(255, 255, 255);
				else
					setColor(200, 200, 255);
				hqFillRoundedRect(0, 0, sx, sy, 10);
			}
			hqEnd();
			
			setColor(0, 0, 100);
			drawText(sx/2, sy/2, 20, 0, 0, "%s", text.c_str());
		}
		gxPopMatrix();
	}
};

static std::map<std::string, ButtonState> buttonStates;

static bool menuTick;
static bool menuDraw;
static float menuDt;

static bool doButton(const char * name)
{
	bool clicked = false;
	
	auto i = buttonStates.find(name);
	
	if (i == buttonStates.end())
	{
		ButtonState state(name);
		
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
	
	return clicked;
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
			
			float ax = 0.f;
			float ay = 0.f;
			
			for (auto & otheri : buttonStates)
			{
				auto &other = otheri.second;
				
				if (&b == &other)
					continue;
				
				const float dx = b.x - other.x;
				const float dy = b.y - other.y;
				const float dsSq = dx * dx + dy * dy;
				const float ds = std::sqrtf(dsSq) + .1f;
				
				const float dd = ds - 500.f;
				const float f = - dd / 100.f;
				const float a = f;
				
				ax += (dx / ds) * a;
				ay += (dy / ds) * a;
			}
			
			b.ax = ax;
			b.ay = ay;
		}
		
		for (auto & bi : buttonStates)
		{
			auto & b = bi.second;
			
			b.x += b.ax * dt;
			b.y += b.ay * dt;
		}
	}

	if (doButton("DatGUI"))
		testDatGui();
	if (doButton("Deep Belief SDK"))
		testDeepbelief();
	if (doButton("Dot Detector"))
		testDotDetector();
	if (doButton("Dot Tracker"))
		testDotTracker();
	if (doButton("1D Fourier Analysis"))
		testFourier1d();
	if (doButton("2D Fourier Analysis"))
		testFourier2d();
	if (doButton("CPU-image delay line"))
		testImageCpuDelayLine();
	if (doButton("Drawing Primitives"))
		testHqPrimitives();
	if (doButton("Impulse Response measurement"))
		testImpulseResponseMeasurement();
	if (doButton("MSDFGEN"))
		testMsdfgen();
	if (doButton("NanoVG"))
		testNanovg();
	if (doButton("STB TrueType"))
		testStbTruetype();
	if (doButton("Texture Atlas"))
		testTextureAtlas();
	if (doButton("Threading"))
		testThreading();
	if (doButton("XMM Gesture Follower"))
		testXmm();

	return doButton("Quit");
}

void testMain()
{
	Surface surface(GFX_SX, GFX_SY, true);
	
	bool stop = false;
	
	do
	{
		framework.process();

		const float dt = framework.timeStep;

		//

		stop = doMenus(true, false, dt);

		//

		framework.beginDraw(0, 0, 0, 0);
		{
			pushSurface(&surface);
			{
				surface.clear();
				
				doMenus(false, true, dt);
			}
			popSurface();
			
			surface.invert();
			
			setColor(colorWhite);
			surface.blit(BLEND_OPAQUE);
		}
		framework.endDraw();
	} while (stop == false);
}
