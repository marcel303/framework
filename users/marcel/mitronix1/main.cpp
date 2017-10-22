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
#if DEVMODE && 0
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

static void playMenuSound();

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
					
					playMenuSound();
				}
			}
			else
			{
				if (hover == true)
				{
					hover = false;
					
					playMenuSound();
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
						
						playMenuSound();
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
		Color color;
		std::string caption;
		
		bool hover;
		bool isDown;
		bool clicked;
		
		Button()
			: x(0.f)
			, y(0.f)
			, radius(0.f)
			, color()
			, caption()
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
						
						playMenuSound();
					}
				}
				else
				{
					if (hover == true)
					{
						hover = false;
						
						playMenuSound();
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
							
							playMenuSound();
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
					setColor(color.mulRGB(.5f));
				else if (hover)
					setColor(color.addRGB(colorWhite.mulRGB(.5f)));
				else
					setColor(color);
				hqFillCircle(0, 0, radius);
				hqEnd();
				
				setFont("calibri.ttf");
				pushFontMode(FONT_SDF);
				{
					setColor(colorWhite);
					drawText(0, 0, 24, 0, 0, "%s", caption.c_str());
				}
				popFontMode();
				
				gxPopMatrix();
			}
		
			return clicked;
		}
	};
	
	struct Slider
	{
		float x;
		float y;
		float sx;
		float sy;
		float radius;
		Color color;
		std::string caption;
		
		float updateSpeed;
		double desiredValue;
		double currentValue;
		
		bool inside;
		bool down;
		
		Slider()
			: x(0.f)
			, y(0.f)
			, sx(100.f)
			, sy(100.f)
			, radius(12.f)
			, color(colorBlue)
			, caption()
			, updateSpeed(1.f)
			, desiredValue(0.0)
			, currentValue(0.0)
			, inside(false)
			, down(false)
		{
		
		}
		
		void process(const bool tick, const bool draw, const float dt)
		{
			if (tick)
			{
				const double retain = std::pow(1.0 - updateSpeed, dt);
				
				currentValue = currentValue * retain + desiredValue * (1.0 - retain);
				
				//
				
				inside = mouse.x >= x && mouse.y >= y && mouse.x <= x + sx && mouse.y <= y + sy;
				
				//
				
				if (inside && mouse.wentDown(BUTTON_LEFT))
					down = true;
				
				if (down && mouse.wentUp(BUTTON_LEFT))
					down = false;
				
				if (down)
				{
					desiredValue = (mouse.x - x) / double(sx);
					
					if (desiredValue < 0.0)
						desiredValue = 0.0;
					if (desiredValue > 1.0)
						desiredValue = 1.0;
				}
			}
			
			if (draw)
			{
				hqBegin(HQ_FILLED_ROUNDED_RECTS);
				{
					setColor(color.mulRGB(1.f/3.f));
					hqFillRoundedRect(x, y, x + sx, y + sy, radius);
					
					setColor(color);
					hqFillRoundedRect(x, y, x + sx * currentValue, y + sy, radius);
				}
				hqEnd();
				
				setFont("calibri.ttf");
				pushFontMode(FONT_SDF);
				{
					setColor(colorWhite);
					drawText(x + sx/2, y + sy/2, 24, 0, 0, "%s", caption.c_str());
				}
				popFontMode();
			}
		}
	};
	
	Button backButton;
	
	Button recordBeginButton;
	Button recordEndButton;
	Button playBeginButton;
	//Button playEndButton;
	Slider playSpeedSlider;
	
	bool showRecordAndPlayback;
	
	InstrumentButtons()
		: backButton()
		, recordBeginButton()
		, recordEndButton()
		, playBeginButton()
		//, playEndButton()
		, showRecordAndPlayback(false)
	{
		backButton.radius = 34;
		backButton.x = GFX_SX - 34*3/2;
		backButton.y = GFX_SY - 34*3/2;
		backButton.color = Color(200, 0, 0);
		backButton.caption = "Back";
		
		//
		
		int x = 34*3/2;
		
		recordBeginButton.radius = 34;
		recordBeginButton.x = x;
		recordBeginButton.y = GFX_SY - 34*3/2;
		recordBeginButton.color = Color(200, 0, 0);
		recordBeginButton.caption = "Rec";
		
		x += 75;
		recordEndButton.radius = 34;
		recordEndButton.x = x;
		recordEndButton.y = GFX_SY - 34*3/2;
		recordEndButton.color = Color(0, 0, 0);
		recordEndButton.caption = "Stop";
		
		x += 75;
		playBeginButton.radius = 34;
		playBeginButton.x = x;
		playBeginButton.y = GFX_SY - 34*3/2;
		playBeginButton.color = Color(200, 200, 0);
		playBeginButton.caption = "Play";
		
		x += 60;
		playSpeedSlider.x = x;
		playSpeedSlider.y = GFX_SY - 34*3/2;
		playSpeedSlider.sx = 300;
		playSpeedSlider.sy = 60;
		playSpeedSlider.radius = 10.f;
		playSpeedSlider.color = colorBlue;
		playSpeedSlider.caption = "Playback speed";
		playSpeedSlider.desiredValue = 1.0;
		playSpeedSlider.updateSpeed = 0.9;
		playSpeedSlider.y -= playSpeedSlider.sy/2;
	}
	
	int process(AudioGraph * audioGraph, const bool tick, const bool draw, const float dt, bool & inputIsCaptured)
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
		
		if (showRecordAndPlayback)
		{
			if (recordBeginButton.process(tick, draw, dt))
			{
				audioGraph->triggerEvent("record-begin");
			}
			
			if (recordEndButton.process(tick, draw, dt))
			{
				audioGraph->triggerEvent("record-end");
				
				audioGraph->triggerEvent("play-end");
			}
			
			if (playBeginButton.process(tick, draw, dt))
			{
				audioGraph->triggerEvent("record-end");
				
				audioGraph->triggerEvent("play-begin");
			}
			
			/*
			if (playEndButton.process(tick, draw, dt))
			{
				audioGraph->triggerEvent("play-end");
			}
			*/
			
			playSpeedSlider.process(tick, draw, dt);
			
			inputIsCaptured |= playSpeedSlider.down;
			
			if (tick)
			{
				audioGraph->setMemf("playSpeed", playSpeedSlider.currentValue);
			}
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

//

#include "soundmix.h"
#include "vfxNodes/delayLine.h"

struct AudioNodeRecordPlay : AudioNodeBase
{
	static const int kRecordBufferSize = SAMPLE_RATE * 60 * 10;
	
	enum Input
	{
		kInput_Audio,
		kInput_RecordBegin,
		kInput_RecordEnd,
		kInput_PlayBegin,
		kInput_PlayEnd,
		kInput_PlaySpeed,
		kInput_COUNT
	};
	
	enum Output
	{
		kOutput_Audio,
		kOutput_COUNT
	};
	
	AudioFloat audioOutput;
	
	bool isRecording;
	bool isPlaying;
	
	float recordBuffer[kRecordBufferSize];
	int recordBufferSize;
	
	double playTime;
	
	AudioNodeRecordPlay()
		: AudioNodeBase()
		, audioOutput(0.f)
		, isRecording(false)
		, isPlaying(false)
		, recordBufferSize(0)
		, playTime(0.0)
	{
		resizeSockets(kInput_COUNT, kOutput_COUNT);
		addInput(kInput_Audio, kAudioPlugType_FloatVec);
		addInput(kInput_RecordBegin, kAudioPlugType_Trigger);
		addInput(kInput_RecordEnd, kAudioPlugType_Trigger);
		addInput(kInput_PlayBegin, kAudioPlugType_Trigger);
		addInput(kInput_PlayEnd, kAudioPlugType_Trigger);
		addInput(kInput_PlaySpeed, kAudioPlugType_FloatVec);
		addOutput(kOutput_Audio, kAudioPlugType_FloatVec, &audioOutput);
	}
	
	virtual void tick(const float dt) override
	{
		const AudioFloat * audio = getInputAudioFloat(kInput_Audio, nullptr);
		const float playSpeed = getInputAudioFloat(kInput_PlaySpeed, &AudioFloat::One)->getMean();
		
		if (isRecording && audio != nullptr)
		{
			audio->expand();
			
			for (int i = 0; i < AUDIO_UPDATE_SIZE; ++i)
			{
				if (recordBufferSize < kRecordBufferSize)
				{
					recordBuffer[recordBufferSize] = audio->samples[i];
					
					recordBufferSize++;
				}
			}
		}
		
		if (isPlaying)
		{
			const double step = 1.0 / SAMPLE_RATE * playSpeed;
			
			audioOutput.setVector();
			
			for (int i = 0; i < AUDIO_UPDATE_SIZE; ++i)
			{
				const double sampleIndex = playTime * SAMPLE_RATE;
				
				const int sampleIndex1 = int(sampleIndex);
				const int sampleIndex2 = int(sampleIndex) + 1;
				
				if (sampleIndex1 >= 0 && sampleIndex1 < recordBufferSize && sampleIndex2 >= 0 && sampleIndex2 < recordBufferSize)
				{
					const float sampleValue1 = recordBuffer[sampleIndex1];
					const float sampleValue2 = recordBuffer[sampleIndex2];
					
					const float t = float(sampleIndex - sampleIndex1);
					
					audioOutput.samples[i] = sampleValue1 * (1.0 - t) + sampleValue2 * t;
				}
				else
				{
					audioOutput.samples[i] = 0.f;
				}
				
				//
				
				playTime += step;
			}
		}
		else
		{
			audioOutput.setScalar(0.f);
		}
	}
	
	virtual void handleTrigger(const int index) override
	{
		if (index == kInput_RecordBegin)
		{
			isRecording = true;
			isPlaying = false;
			
			recordBufferSize = 0;
		}
		else if (index == kInput_RecordEnd)
		{
			if (isRecording)
			{
				isRecording = true;
				Assert(isPlaying == false);
			}
		}
		else if (index == kInput_PlayBegin)
		{
			isRecording = false;
			isPlaying = true;
			
			playTime = 0.f;
		}
		else if (index == kInput_PlayEnd)
		{
			if (isPlaying)
			{
				Assert(isRecording == false);
				isPlaying = false;
			}
		}
	}
};

AUDIO_NODE_TYPE(record_play, AudioNodeRecordPlay)
{
	typeName = "recordAndPlay";
	
	in("audio", "audioValue");
	in("recordBegin", "trigger");
	in("recordEnd", "trigger");
	in("play", "trigger");
	in("playEnd", "trigger");
	in("playSpeed", "audioValue");
	out("audio", "audioValue");
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

static void playMenuSound()
{
#if DEVMODE
	Sound("menuselect.ogg").play();
#endif
}

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
		
		Instrument * instrument = nullptr;
		
		InstrumentButtons instrumentButtons;
		
		auto selectInstrument = [&](const int index)
		{
			delete instrument;
			instrument = nullptr;
			
			instrumentButtons.showRecordAndPlayback = false;
			
			if (index == 0)
				instrument = new Instrument("mtx1.xml");
			if (index == 1)
				instrument = new Instrument("mtx2.xml");
			if (index == 2)
			{
				instrument = new Instrument("mtx3.xml");
				
				instrumentButtons.showRecordAndPlayback = true;
			}
			
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
			
			//
			
			bool inputIsCaptured = false;
			
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
				const int button = instrumentButtons.process(instrument->audioGraphInstance->audioGraph, true, false, dt, inputIsCaptured);
				
				if (button == 0)
				{
					blurStrength = 1.0;
					
					view = kView_MainButtons;
					mainButtons = MainButtons();
					
					selectInstrument(-1);
				}
			}
			
			if (view == kView_MainButtons)
				mainButtonsOpacity = std::min(1.0, mainButtonsOpacity + dt / 1.0);
			else
				mainButtonsOpacity = std::max(0.0, mainButtonsOpacity - dt / 1.0);
			
			blurStrength *= std::pow(.01, double(dt * 4.0));
			
		#if DEVMODE == 0
			if (audioGraphMgr->selectedFile != nullptr)
			{
				audioGraphMgr->selectedFile->graphEdit->flags = GraphEdit::kFlag_Drag*0 | GraphEdit::kFlag_Zoom*0 | GraphEdit::kFlag_ToggleIsPassthrough | GraphEdit::kFlag_ToggleIsFolded;
				
				audioGraphMgr->selectedFile->graphEdit->editorOptions.showBackground = true;
				audioGraphMgr->selectedFile->graphEdit->editorOptions.backgroundColor = ParticleColor(0.f, 0.f, 0.f, .5f);
			}
		#endif
		
			inputIsCaptured |= audioGraphMgr->tickEditor(dt, inputIsCaptured);
			
			slideshow.tick(dt);
			
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
						
						instrumentButtons.process(nullptr, false, true, dt, inputIsCaptured);
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
