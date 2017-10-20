#include "audioGraph.h"
#include "audioGraphManager.h"
#include "audioUpdateHandler.h"
#include "audioNodeBase.h"
#include "framework.h"
#include "graph.h"
#include "slideshow.h"
#include "soundmix.h"

#define DEVMODE 0

extern const int GFX_SX;
extern const int GFX_SY;
#if DEVMODE
const int GFX_SX = 800;
const int GFX_SY = 700;
#else
const int GFX_SX = 800;
const int GFX_SY = 300;
#endif

static SDL_Cursor * handCursor = nullptr;

enum View
{
	kView_MainButtons,
	kView_Instrument
};

static View view = kView_MainButtons;

static bool buttonPressed = false;
static bool hasMouseHover = false;

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
		, desiredX(0.f)
		, desiredY(0.f)
		, sx(220)
		, sy(50.f)
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
				const float border = 4.f;
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
				drawText(sx/2, sy/2, 36, 0, 0, "%s", caption.c_str());
			}
			popFontMode();
			
			gxPopMatrix();
		}
		
		return clicked;
	}
};

struct MainButtons
{
	MainButton button1;
	MainButton button2;
	MainButton button3;
	
	MainButtons()
		: button1()
		, button2()
		, button3()
	{
		const int numButtons = 3;
		const int buttonSx = button1.sx;
		const int buttonSpacing = 30;
		const int emptySpace = GFX_SX - buttonSpacing * (numButtons - 1) - buttonSx * numButtons;
		
		int x = emptySpace/2;
		
		button1.caption = "Loop";
		button1.currentX = x - 40;
		button1.currentY = GFX_SY * 2/3;
		button1.desiredX = x;
		button1.desiredY = button1.currentY;
		x += buttonSx;
		x += buttonSpacing;
		
		button2.caption = "Play";
		button2.currentX = x - 40;
		button2.currentY = GFX_SY * 2/3;
		button2.desiredX = x;
		button2.desiredY = button2.currentY;
		x += buttonSx;
		x += buttonSpacing;
		
		button3.caption = "Echo";
		button3.currentX = GFX_SX - button3.sx - 100.f;
		button3.currentY = GFX_SY * 2/3;
		button3.desiredX = x;
		button3.desiredY = button3.currentY;
		x += buttonSx;
		x += buttonSpacing;
	}
	
	int process(const bool tick, const bool draw, const float dt)
	{
		int result = -1;
		
		if (tick)
		{
			hasMouseHover = false;
			buttonPressed = false;
		}
		
		if (button1.process(tick, draw, dt))
		{
			result = 0;
		}
		
		if (button2.process(tick, draw, dt))
		{
			result = 1;
		}
		
		if (button3.process(tick, draw, dt))
		{
			result = 2;
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

struct InstrumentButtons
{
	struct Button
	{
		float x;
		float y;
		float radius;
		
		bool hover;
		bool isDown;
		bool clicked;
		
		Button()
			: x(0.f)
			, y(0.f)
			, radius(0.f)
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
				const float dx = mouse.x - x;
				const float dy = mouse.y - y;
				const float distance = std::hypot(dx, dy);
				
				const bool isInside = distance < radius;
				
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
				gxTranslatef(x, y, 0);
				
				if (hover)
				{
					hqBegin(HQ_STROKED_CIRCLES);
					setColor(colorWhite);
					hqStrokeCircle(0, 0, radius, 4.f);
					hqEnd();
				}
				
				hqBegin(HQ_FILLED_CIRCLES);
				if (isDown)
					setColor(100, 0, 0);
				else if (hover)
					setColor(200, 100, 100);
				else
					setColor(200, 0, 0);
				hqFillCircle(0, 0, radius);
				hqEnd();
				
				setFont("calibri.ttf");
				pushFontMode(FONT_SDF);
				{
					setColor(colorWhite);
					drawText(0, 0, 24, 0, 0, "%s", "Back");
				}
				popFontMode();
				
				gxPopMatrix();
			}
		
			return clicked;
		}
	};
	
	Button backButton;
	
	InstrumentButtons()
		: backButton()
	{
		backButton.radius = 34;
		backButton.x = GFX_SX - 34*3/2;
		backButton.y = GFX_SY - 34*3/2;
	}
	
	int process(const bool tick, const bool draw, const float dt)
	{
		int result = -1;
		
		if (tick)
		{
			hasMouseHover = false;
			buttonPressed = false;
		}
		
		if (backButton.process(tick, draw, dt))
		{
			result = 0;
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

struct MitronixValues
{
	AudioFloat mouseX;
	AudioFloat mouseY;
	
	MitronixValues()
		: mouseX(0.f)
		, mouseY(0.f)
	{
	}
};

static MitronixValues g_mitronixValues;

struct AudioNodeMitronix : AudioNodeBase
{
	enum Input
	{
		kInput_COUNT
	};
	
	enum Output
	{
		kOutput_MouseX,
		kOutput_MouseY,
		kOutput_COUNT
	};
	
	AudioNodeMitronix()
		: AudioNodeBase()
	{
		resizeSockets(kInput_COUNT, kOutput_COUNT);
		addOutput(kOutput_MouseX, kAudioPlugType_FloatVec, &g_mitronixValues.mouseX);
		addOutput(kOutput_MouseY, kAudioPlugType_FloatVec, &g_mitronixValues.mouseY);
	}
};

AUDIO_NODE_TYPE(mitronix, AudioNodeMitronix)
{
	typeName = "mitronix";
	
	out("mouse.x", "audioValue");
	out("mouse.y", "audioValue");
}

struct Instrument
{
	AudioGraphInstance * audioGraphInstance;
	
	Instrument(const char * filename)
		: audioGraphInstance(nullptr)
	{
		audioGraphInstance = g_audioGraphMgr->createInstance(filename);
	}
	
	~Instrument()
	{
		g_audioGraphMgr->free(audioGraphInstance);
	}
};

int main(int argc, char * argv[])
{
	if (framework.init(0, 0, GFX_SX, GFX_SY))
	{
	#if DEVMODE == 0
		framework.fillCachesWithPath(".", true);
	#endif
		
		fillHrirSampleSetCache("cipic.147", "cipic.147", kHRIRSampleSetType_Cipic);
		
		handCursor = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_HAND);
		
		Surface * surface = new Surface(GFX_SX, GFX_SY, true);
		
		SDL_mutex * audioMutex = SDL_CreateMutex();
		
		AudioVoiceManager * audioVoiceMgr = new AudioVoiceManager();
		audioVoiceMgr->init(16, 16);
		audioVoiceMgr->outputStereo = true;
		g_voiceMgr = audioVoiceMgr;
		
		AudioGraphManager * audioGraphMgr = new AudioGraphManager();
		audioGraphMgr->init(audioMutex);
		g_audioGraphMgr = audioGraphMgr;
		
		AudioUpdateHandler * audioUpdateHandler = new AudioUpdateHandler();
		audioUpdateHandler->init(audioMutex, nullptr, 0);
		audioUpdateHandler->voiceMgr = audioVoiceMgr;
		audioUpdateHandler->audioGraphMgr = audioGraphMgr;
		
		PortAudioObject * paObject = new PortAudioObject();
		if (paObject->init(SAMPLE_RATE, 2, 2, AUDIO_UPDATE_SIZE, audioUpdateHandler) == false)
			logError("failed to initialize port audio object");
		
		Slideshow slideshow;
		slideshow.pics = { "001.jpg", "002.jpg", "003.jpg", "004.jpg", "005.jpg", "006.jpg", "007.jpg" };
		
		// main view
		
		MainButtons mainButtons;
		
		Surface * mainButtonsSurface = new Surface(GFX_SX, GFX_SY, true);
		
		double mainButtonsOpacity = 0.0;
		
		// instrument view
		
		Instrument * instrument = new Instrument("mtx1.xml");
		audioGraphMgr->selectInstance(instrument->audioGraphInstance);
		
		InstrumentButtons instrumentButtons;
		
		auto selectInstrument = [&](const int index)
		{
			delete instrument;
			instrument = nullptr;
			
			if (index == 0)
				instrument = new Instrument("mtx1.xml");
			if (index == 1)
				instrument = new Instrument("mtx2.xml");
			if (index == 2)
				instrument = new Instrument("mtx3.xml");
			
			if (instrument != nullptr)
				audioGraphMgr->selectInstance(instrument->audioGraphInstance);
		};
		
		//
		
		bool stop = false;
		
		double blurStrength = 0.0;
		
		do
		{
			framework.process();
			
			if (framework.quitRequested)
				stop = true;
			if (keyboard.wentDown(SDLK_ESCAPE))
				stop = true;
			
			const float dt = std::min(1.f / 15.f, framework.timeStep);
			
			MitronixValues values;
			values.mouseX = mouse.x / float(GFX_SX);
			values.mouseY = mouse.y / float(GFX_SY);;
			
			SDL_LockMutex(audioMutex);
			{
				g_mitronixValues = values;
			}
			SDL_UnlockMutex(audioMutex);
			
			slideshow.tick(dt);
			
		#if DEVMODE == 0
			if (audioGraphMgr->selectedFile != nullptr)
				audioGraphMgr->selectedFile->graphEdit->flags = GraphEdit::kFlag_Drag | GraphEdit::kFlag_Zoom | GraphEdit::kFlag_ToggleIsPassthrough | GraphEdit::kFlag_ToggleIsFolded;
		#endif
		
			audioGraphMgr->tickEditor(dt, false);
			
			if (view == kView_MainButtons)
			{
				const int button = mainButtons.process(true, false, dt);
				
				if (button != -1)
				{
					selectInstrument(button);
					
					blurStrength = 1.0;
					
					view = kView_Instrument;
				}
			}
			else if (view == kView_Instrument)
			{
				const int button = instrumentButtons.process(true, false, dt);
				
				if (button == 0)
				{
					blurStrength = 1.0;
					
					view = kView_MainButtons;
					mainButtons = MainButtons();
				}
			}
			
			if (view == kView_MainButtons)
				mainButtonsOpacity = std::min(1.0, mainButtonsOpacity + dt / 1.0);
			else
				mainButtonsOpacity = std::max(0.0, mainButtonsOpacity - dt / 1.0);
			
			blurStrength *= std::pow(.01, double(dt * 4.0));
			
			framework.beginDraw(0, 0, 0, 0);
			{
				pushSurface(surface);
				{
					surface->clear(220, 220, 220);
					
					if (view == kView_Instrument)
					{
					#if DEVMODE == 0
						pushBlend(BLEND_OPAQUE);
						setColor(colorWhite);
						slideshow.draw();
						popBlend();
					#endif
						
						audioGraphMgr->drawEditor();
						
						instrumentButtons.process(false, true, dt);
					}
					
					pushSurface(mainButtonsSurface);
					{
						mainButtonsSurface->clear(0, 0, 0, 0);
						mainButtons.process(false, true, dt);
					}
					popSurface();
					
					pushBlend(BLEND_ALPHA);
					gxSetTexture(mainButtonsSurface->getTexture());
					setColorf(1, 1, 1, mainButtonsOpacity);
					drawRect(0, 0, mainButtonsSurface->getWidth(), mainButtonsSurface->getHeight());
					gxSetTexture(0);
					popBlend();
				}
				popSurface();
				
				surface->gaussianBlur(
					blurStrength * 100.f,
					blurStrength * 100.f,
					std::ceilf(blurStrength * 100.f));
				
				setColor(colorWhite);
				surface->blit(BLEND_OPAQUE);
			}
			framework.endDraw();
		} while (stop == false);
		
		delete instrument;
		instrument = nullptr;
		
		delete mainButtonsSurface;
		mainButtonsSurface = nullptr;
		
		paObject->shut();
		delete paObject;
		paObject = nullptr;
		
		audioUpdateHandler->shut();
		delete audioUpdateHandler;
		audioUpdateHandler = nullptr;
		
		g_audioGraphMgr = nullptr;
		audioGraphMgr->shut();
		delete audioGraphMgr;
		audioGraphMgr = nullptr;
		
		g_voiceMgr = nullptr;
		audioVoiceMgr->shut();
		delete audioVoiceMgr;
		audioVoiceMgr = nullptr;
		
		SDL_DestroyMutex(audioMutex);
		audioMutex = nullptr;
		
		delete surface;
		surface = nullptr;
		
		SDL_FreeCursor(handCursor);
		handCursor = nullptr;
		
		Font("calibri.ttf").saveCache();
		
		framework.shutdown();
	}
	
	return 0;
}
