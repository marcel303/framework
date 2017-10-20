#include "audioGraphManager.h"
#include "audioUpdateHandler.h"
#include "framework.h"
#include "graph.h"
#include "slideshow.h"
#include "soundmix.h"

#define DEVMODE 1

extern const int GFX_SX;
extern const int GFX_SY;
const int GFX_SX = 800;
const int GFX_SY = 300;

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
		const int numButtons = 2;
		const int buttonSx = button1.sx;
		const int buttonSpacing = 30;
		const int emptySpace = GFX_SX - buttonSpacing * (numButtons - 1) - buttonSx * numButtons;
		
		int x = emptySpace/2 - buttonSx/2;
		
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
		framework.fillCachesWithPath(".", true);
		
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
		
		MainButtons mainButtons;
		Surface * mainButtonsSurface = new Surface(GFX_SX, GFX_SY, true);
		double mainButtonsOpacity = 0.0;
		
		Instrument * instrument = new Instrument("mtx1.xml");
		audioGraphMgr->selectInstance(instrument->audioGraphInstance);
		
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
		};
	
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
			
			slideshow.tick(dt);
			
			if (instrument != nullptr)
				audioGraphMgr->selectInstance(instrument->audioGraphInstance);
		#if DEVMODE == 0
			if (audioGraphMgr->selectedFile != nullptr)
				audioGraphMgr->selectedFile->graphEdit->flags = GraphEdit::kFlag_Drag | GraphEdit::kFlag_Zoom;
		#endif
		
			audioGraphMgr->tickEditor(dt, false);
			
			if (view == kView_MainButtons)
			{
				const int button = mainButtons.process(true, false, dt);
				
				if (button != -1)
				{
					selectInstrument(button);
					
					mainButtons = MainButtons();
					
					blurStrength = 1.0;
					
					view = kView_Instrument;
				}
			}
			else if (view == kView_Instrument)
			{
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
					surface->clear();
					
					if (view == kView_Instrument)
					{
						pushBlend(BLEND_OPAQUE);
						setColor(colorWhite);
						slideshow.draw();
						popBlend();
						
						audioGraphMgr->drawEditor();
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
