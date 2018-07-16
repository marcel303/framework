#include "framework.h"

#include "jsusfx.h"
#include "jsusfx_file.h"
#include "jsusfx_gfx.h"

#include "gfx-framework.h"
#include "jsusfx-framework.h"

#include "Path.h"
#include "StringEx.h"

#include <map>
#include <vector>

/*

GFX status:
- All Liteon scripts appear to work.
- JSFX-Kawa (https://github.com/kawaCat/JSFX-kawa) works.
- ATK for Reaper works.

todo :

mouse_cap

mouse_cap is a bitfield of mouse and keyboard modifier state.
	1: left mouse button
	2: right mouse button
	4: Control key (Windows), Command key (OSX)
	8: Shift key
	16: Alt key (Windows), Option key (OSX)
	32: Windows key (Windows), Control key (OSX) -- REAPER 4.60+
	64: middle mouse button -- REAPER 4.60+

mouse_wheel

gfx_mode
	0x1 = additive
	0x2 = disable source alpha for gfx_blit (make it opaque)
	0x4 = disable filtering for gfx_blit (point sampling)

*/

/*

test todo :

- add octave select to on-screen MIDI keyboard
- bring JSFX window to top when selecting it in the effect chain window
- draw a cross when an effect is active in the effect chain list
- show CPU-usage per effect (perhaps in the effect chain list?)
- show effect stats like number of I/O pins, description, sliders when hovering over an effect in the effect chain list
- don't draw JSFX UI when a window is minimized
- add an option to hide to JSFX UI from the effect chain list. perhaps hide should be the default behavior of the close button, instead of closing and removing it from the effect chain?
- add an effect search filter box

*/

#define SAMPLE_RATE 44100
#define BUFFER_SIZE 64

#define DATA_ROOT "/Users/thecat/Library/Application Support/REAPER/Data/"

#define SEARCH_PATH_reaper "/Users/thecat/Library/Application Support/REAPER/Effects/"
#define SEARCH_PATH_geraintluff "/Users/thecat/geraintluff -jsfx/"
#define SEARCH_PATH_ATK "/Users/thecat/atk-reaper/plugins/"
#define SEARCH_PATH_kawa "/Users/thecat/Downloads/JSFX-kawa-master/"

#define RENDER_TO_SURFACE 0
#define ENABLE_SCREENSHOT_API 0

const int GFX_SX = 1000;
const int GFX_SY = 720;

static void doSlider(JsusFx & fx, JsusFx_Slider & slider, int x, int y, bool & isActive)
{
	const int sx = 200;
	const int sy = 12;
	
	const bool isInside =
		x >= 0 && x <= sx &&
		y >= 0 && y <= sy;
	
	if (isInside && mouse.wentDown(BUTTON_LEFT))
		isActive = true;
	
	if (mouse.wentUp(BUTTON_LEFT))
		isActive = false;
	
	if (isActive)
	{
		const float t = x / float(sx);
		const float v = slider.min + (slider.max - slider.min) * t;
		fx.moveSlider(&slider - fx.sliders, v);
	}
	
	setColor(0, 0, 255, 127);
	const float t = (slider.getValue() - slider.min) / (slider.max - slider.min);
	drawRect(0, 0, sx * t, sy);
	
	if (slider.isEnum)
	{
		const int enumIndex = (int)slider.getValue();
		
		if (enumIndex >= 0 && enumIndex < slider.enumNames.size())
		{
			setColor(colorWhite);
			drawText(sx/2.f, sy/2.f, 10.f, 0.f, 0.f, "%s", slider.enumNames[enumIndex].c_str());
		}
	}
	else
	{
		setColor(colorWhite);
		drawText(sx/2.f, sy/2.f, 10.f, 0.f, 0.f, "%s", slider.desc);
	}
	
	setColor(63, 31, 255, 127);
	drawRectLine(0, 0, sx, sy);
}

//

#define MIDI_OFF 0x80
#define MIDI_ON 0x90

struct MidiKeyboard
{
	static const int kNumKeys = 16;
	
	struct Key
	{
		int note;
		bool isDown;
	};
	
	Key keys[kNumKeys];
	
	MidiKeyboard()
	{
		for (int i = 0; i < kNumKeys; ++i)
		{
			auto & key = keys[i];
			
			key.note = 32 + i;
			key.isDown = false;
		}
	}
};

static void writeMidi(uint8_t *& midi, int & midiSize, const int message, const int param1, const int param2)
{
	*midi++ = message;
	*midi++ = param1;
	*midi++ = param2;
	
	midiSize += 3;
}

void doMidiKeyboard(MidiKeyboard & kb, const int mouseX, const int mouseY, uint8_t * midi, int * midiSize, const bool doTick, const bool doDraw)
{
	const int keySx = 16;
	const int keySy = 64;

	const int sx = keySx * MidiKeyboard::kNumKeys;
	const int sy = keySy;

	const int hoverIndex = mouseY >= 0 && mouseY <= keySy ? mouseX / keySx : -1;
	const float velocity = clamp(mouseY / float(keySy), 0.f, 1.f);
	
	if (doTick)
	{
		bool isDown[MidiKeyboard::kNumKeys];
		memset(isDown, 0, sizeof(isDown));
		
		if (mouse.isDown(BUTTON_LEFT) && hoverIndex >= 0 && hoverIndex < MidiKeyboard::kNumKeys)
			isDown[hoverIndex] = true;
		
		for (int i = 0; i < MidiKeyboard::kNumKeys; ++i)
		{
			auto & key = kb.keys[i];
			
			if (key.isDown != isDown[i])
			{
				if (isDown[i])
				{
					key.isDown = true;
					
					writeMidi(midi, *midiSize, MIDI_ON, key.note, velocity * 127);
				}
				else
				{
					writeMidi(midi, *midiSize, MIDI_OFF, key.note, velocity * 127);
					
					key.isDown = false;
				}
			}
		}
	}
	
	if (doDraw)
	{
		setColor(colorBlack);
		drawRect(0, 0, sx, sy);
		
		const Color colorKey(200, 200, 200);
		const Color colorkeyHover(255, 255, 255);
		const Color colorKeyDown(100, 100, 100);
		
		hqSetGradient(GRADIENT_LINEAR, Mat4x4(true).RotateZ(M_PI/2.f).Scale(1.f, 1.f / keySy, 1.f), Color(140, 180, 220), colorWhite, COLOR_MUL);
		
		for (int i = 0; i < MidiKeyboard::kNumKeys; ++i)
		{
			auto & key = kb.keys[i];
			
			gxPushMatrix();
			{
				gxTranslatef(keySx * i, 0, 0);
				
				setColor(key.isDown ? colorKeyDown : i == hoverIndex ? colorkeyHover : colorKey);
				
				hqBegin(HQ_FILLED_RECTS);
				hqFillRect(0, 0, keySx, keySy);
				hqEnd();
			}
			gxPopMatrix();
		}
		
		hqClearGradient();
	}
}

//

#include "audiostream/AudioOutput_PortAudio.h"
#include "audiostream/AudioStream.h"
#include "audiostream/AudioStreamVorbis.h"

struct MidiBuffer
{
	static const int kMaxBytes = 1000 * 3;
	
	uint8_t bytes[kMaxBytes];
	int numBytes = 0;
};

static MidiBuffer s_midiBuffer;

//

struct Limiter
{
	double measuredMax = 0.0;
	
	float next(const float value)
	{
		const double mag = fabs(value);
		
		if (mag > measuredMax)
			measuredMax = mag;
		
		if (measuredMax < 1.0)
			return value;
		else
		{
			const float result = value / measuredMax;
			
			measuredMax *= 0.999;
			
			return result;
		}
	}
};

struct AudioStream_JsusFx : AudioStream
{
	AudioStream * source = nullptr;
	
	JsusFx * fx = nullptr;
	
	SDL_mutex * mutex = nullptr;
	
	Limiter limiter;
	
	virtual ~AudioStream_JsusFx() override
	{
		if (mutex != nullptr)
		{
			SDL_DestroyMutex(mutex);
			mutex = nullptr;
		}
	}
	
	void init(AudioStream * _source, JsusFx * _fx)
	{
		source = _source;
		
		fx = _fx;
		
		mutex = SDL_CreateMutex();
	}
	
	void lock()
	{
		const int r = SDL_LockMutex(mutex);
		Assert(r == 0);
		(void)r;
	}
	
	void unlock()
	{
		const int r = SDL_UnlockMutex(mutex);
		Assert(r == 0);
		(void)r;
	}
	
	virtual int Provide(int numSamples, AudioSample* __restrict buffer) override
	{
		Assert(numSamples == BUFFER_SIZE);
		
		source->Provide(numSamples, buffer);
		
		float inputL[BUFFER_SIZE];
		float inputR[BUFFER_SIZE];
		float outputL[BUFFER_SIZE];
		float outputR[BUFFER_SIZE];
		
		for (int i = 0; i < numSamples; ++i)
		{
			inputL[i] = buffer[i].channel[0] / float(1 << 15);
			inputR[i] = buffer[i].channel[1] / float(1 << 15);
		}
		
		const float * input[2] =
		{
			inputL,
			inputR
		};
		
		float * output[2] =
		{
			outputL,
			outputR
		};
		
		lock();
		{
			fx->setMidi(s_midiBuffer.bytes, s_midiBuffer.numBytes);
			
			fx->process(input, output, numSamples, 2, 2);
			
			s_midiBuffer.numBytes = 0;
		}
		unlock();
		
		for (int i = 0; i < numSamples; ++i)
		{
			const float valueL = limiter.next(outputL[i]);
			const float valueR = limiter.next(outputR[i]);
			
			buffer[i].channel[0] = valueL * float((1 << 15) - 2);
			buffer[i].channel[1] = valueR * float((1 << 15) - 2);
		}
		
		return numSamples;
	}
};

//

static JsusFx * s_fx = nullptr;

static AudioStream_JsusFx * s_audioStream = nullptr;

static void handleAction(const std::string & action, const Dictionary & d)
{
	if (action == "filedrop")
	{
		s_audioStream->lock();
		{
			auto filename = d.getString("file", "");
			
			JsusFxPathLibrary_Basic pathLibrary(DATA_ROOT);
			s_fx->compile(pathLibrary, filename);
			
			s_fx->prepare(SAMPLE_RATE, BUFFER_SIZE);
		}
		s_audioStream->unlock();
	}
}

//

struct AudioStream_JsusFxChain : AudioStream
{
	struct EffectElem
	{
		bool isPassthrough = false;
		JsusFx * jsusFx = nullptr;
	};
	
	AudioStream * source = nullptr;
	
	std::vector<EffectElem> effects;
	
	SDL_mutex * mutex = nullptr;
	
	Limiter limiter;
	
	virtual ~AudioStream_JsusFxChain() override
	{
		if (mutex != nullptr)
		{
			SDL_DestroyMutex(mutex);
			mutex = nullptr;
		}
	}
	
	void init(AudioStream * _source)
	{
		source = _source;
		
		mutex = SDL_CreateMutex();
	}
	
	void lock()
	{
		const int r = SDL_LockMutex(mutex);
		Assert(r == 0);
		(void)r;
	}
	
	void unlock()
	{
		const int r = SDL_UnlockMutex(mutex);
		Assert(r == 0);
		(void)r;
	}
	
	void add(JsusFx * jsusFx)
	{
		lock();
		{
			EffectElem elem;
			elem.jsusFx = jsusFx;
			
			effects.push_back(elem);
		}
		unlock();
	}
	
	void remove(JsusFx * jsusFx)
	{
		lock();
		{
			auto i = std::find_if(effects.begin(), effects.end(), [=](const EffectElem & e) -> bool { return e.jsusFx == jsusFx; });
			
			Assert(i != effects.end());
			if (i != effects.end())
				effects.erase(i);
		}
		unlock();
	}
	
	virtual int Provide(int numSamples, AudioSample* __restrict buffer) override
	{
		Assert(numSamples == BUFFER_SIZE);
		
		const int numSourceSamples = source->Provide(numSamples, buffer);
		
		const int kNumBuffers = 4;
		float bufferData[2][kNumBuffers][BUFFER_SIZE];
		memset(bufferData, 0, sizeof(bufferData));
		
		int inputIndex = 0;
		
		for (int i = 0; i < numSourceSamples; ++i)
		{
			bufferData[inputIndex][0][i] = buffer[i].channel[0] / float(1 << 15);
			bufferData[inputIndex][1][i] = buffer[i].channel[1] / float(1 << 15);
		}
		
		for (int i = numSourceSamples; i < numSamples; ++i)
		{
			bufferData[inputIndex][0][i] = 0.f;
			bufferData[inputIndex][1][i] = 0.f;
		}
		
		lock();
		{
			for (auto & effect : effects)
			{
				if (effect.isPassthrough)
					continue;
				
				auto & jsusFx = effect.jsusFx;
				
				jsusFx->setMidi(s_midiBuffer.bytes, s_midiBuffer.numBytes);
				
				const float * input[kNumBuffers];
				float * output[kNumBuffers];
				
				for (int i = 0; i < kNumBuffers; ++i)
				{
					input[i] = bufferData[inputIndex][i];
					output[i] = bufferData[1 - inputIndex][i];
				}
				
				if (jsusFx->process(input, output, numSamples, kNumBuffers, kNumBuffers))
				{
					inputIndex = 1 - inputIndex;
				}
			}
			
			s_midiBuffer.numBytes = 0;
		}
		unlock();
		
		inputIndex = 1 - inputIndex;
		
		for (int i = 0; i < numSamples; ++i)
		{
			const float valueL = limiter.next(bufferData[1 - inputIndex][0][i]);
			const float valueR = limiter.next(bufferData[1 - inputIndex][1][i]);
			
			buffer[i].channel[0] = valueL * float((1 << 15) - 2);
			buffer[i].channel[1] = valueR * float((1 << 15) - 2);
		}
		
		return numSamples;
	}
};

struct JsusFxWindow
{
	Window * window;
#if RENDER_TO_SURFACE
	Surface * surface;
#endif
	
	JsusFxPathLibrary_Basic pathLibrary;
	
	JsusFx_Framework jsusFx;
	JsusFxFileAPI_Basic fileAPI;
	JsusFxGfx_Framework gfxAPI;
	
	std::string filename;
	
	bool isValid;
	bool sliderIsActive[JsusFx::kMaxSliders];
	
	JsusFxWindow(const char * _filename)
		: window(nullptr)
	#if RENDER_TO_SURFACE
		, surface(nullptr)
	#endif
		, pathLibrary(DATA_ROOT)
		, jsusFx(pathLibrary)
		, fileAPI()
		, gfxAPI(jsusFx)
		, filename(_filename)
		, isValid(false)
	{
		memset(sliderIsActive, 0, sizeof(sliderIsActive));
		
		fileAPI.init(jsusFx.m_vm);
		jsusFx.fileAPI = &fileAPI;
		
		gfxAPI.init(jsusFx.m_vm);
		jsusFx.gfx = &gfxAPI;
		
		pathLibrary.addSearchPath("lib");
		
		if (jsusFx.compile(pathLibrary, filename))
		{
			jsusFx.prepare(SAMPLE_RATE, BUFFER_SIZE);
			
			const std::string caption = String::FormatC("%s (%d ins, %d outs)", Path::GetFileName(filename).c_str(), jsusFx.numInputs, jsusFx.numOutputs);
			
			window = new Window(caption.c_str(), jsusFx.gfx_w, jsusFx.gfx_h, true);
			
			isValid = true;
		}
	}
	
	~JsusFxWindow()
	{
	#if RENDER_TO_SURFACE
		delete surface;
		surface = nullptr;
	#endif
		
		delete window;
		window = nullptr;
	}
	
	void tick(const float dt)
	{
		if (!isValid)
			return;
		
		if (window->getQuitRequested())
		{
			delete window;
			window = nullptr;
			
			isValid = false;
		}
	}
	
	void draw()
	{
		if (!isValid)
			return;
		
		pushWindow(*window);
		{
		#if RENDER_TO_SURFACE
			if (surface == nullptr || surface->getWidth() != window->getWidth() || surface->getHeight() != window->getHeight())
			{
				delete surface;
				surface = nullptr;
				
				//
				
				surface = new Surface(window->getWidth(), window->getHeight(), false);
				surface->clear();
			}
		#endif
			
			framework.beginDraw(0, 0, 0, 0);
			{
			#if ENABLE_SCREENSHOT_API
				const bool doScreenshot = keyboard.wentDown(SDLK_s);
				
				if (doScreenshot)
					framework.beginScreenshot(0, 0, 0, 0, 4);
			#endif

			#if !RENDER_TO_SURFACE
				Surface * surface = nullptr;
			#endif

				gfxAPI.setup(surface, window->getWidth(), window->getHeight(), mouse.x, mouse.y, true);
				
				pushFontMode(FONT_SDF);
				setColorClamp(true);
				{
					jsusFx.draw();
				}
				setColorClamp(false);
				popFontMode();
				
			#if RENDER_TO_SURFACE
				surface->blit(BLEND_OPAQUE);
			#endif
				
				int x = 10;
				int y = 10;
				
				int sliderIndex = 0;
				
				for (auto & slider : jsusFx.sliders)
				{
					if (slider.exists && slider.desc[0] != '-')
					{
						gxPushMatrix();
						gxTranslatef(x, y, 0);
						doSlider(jsusFx, slider, mouse.x - x, mouse.y - y, sliderIsActive[sliderIndex]);
						gxPopMatrix();
						
						y += 16;
					}
					
					sliderIndex++;
				}
				
				x += 300;
				
				const int sx = std::max(x, window->getWidth());
				const int sy = std::max(y, window->getHeight());
				
				if (sx > window->getWidth() || sy > window->getHeight())
				{
					window->setSize(sx, sy);
				}
				
			#if ENABLE_SCREENSHOT_API
				if (doScreenshot)
					framework.endScreenshot("jsusfx-screenshot%04d");
			#endif
			}
			framework.endDraw();
		}
		popWindow();
	}
};

struct JsusFxChainWindow
{
	Window window;
	
	AudioStream_JsusFxChain & effectChain;
	
	int selectedEffectIndex;
	
	JsusFxChainWindow(AudioStream_JsusFxChain & _effectChain)
		: window("Effect Chain", 300, 300, true)
		, effectChain(_effectChain)
		, selectedEffectIndex(0)
	{
	}
	
	void tick(const float dt)
	{
		pushWindow(window);
		{
			framework.beginDraw(50, 50, 50, 0);
			{
				setFont("calibri.ttf");
				setColor(colorWhite);
				
				int x = 10;
				int y = 10;
				
				int effectIndex = 0;
				
				for (auto & effect : effectChain.effects)
				{
					{
						const int x1 = x;
						const int y1 = y;
						const int x2 = x + 16;
						const int y2 = y + 16;
						
						const bool isInside =
							mouse.x >= x1 &&
							mouse.y >= y1 &&
							mouse.x <= x2 &&
							mouse.y <= y2;
						
						if (isInside && mouse.wentDown(BUTTON_LEFT))
							effect.isPassthrough = !effect.isPassthrough;
						
						setLumi(effect.isPassthrough ? 0 : 200);
						drawRect(x1, y1, x2, y2);
						setLumi(100);
						drawRectLine(x1, y1, x2, y2);
					}
					
					{
						const int x1 = x + 20;
						const int y1 = y;
						const int x2 = window.getWidth();
						const int y2 = y + 16;
						
						const bool isInside =
							mouse.x >= x1 &&
							mouse.y >= y1 &&
							mouse.x <= x2 &&
							mouse.y <= y2;
						
						if (isInside && mouse.wentDown(BUTTON_LEFT))
							selectedEffectIndex = effectIndex;
						
						if (effectIndex == selectedEffectIndex)
						{
							setLumi(0);
							drawRect(x1, y1, x2, y2);
						}
						
						setLumi(200);
						drawText(x1, (y1 + y2)/2, 14, +1, 0, "%s", effect.jsusFx->desc);
						y += 20;
					}
					
					effectIndex++;
				}
				
				effectChain.lock();
				{
					if (keyboard.wentDown(SDLK_UP) && selectedEffectIndex - 1 >= 0)
					{
						std::swap(effectChain.effects[selectedEffectIndex], effectChain.effects[selectedEffectIndex - 1]);
						selectedEffectIndex--;
					}
					
					if (keyboard.wentDown(SDLK_DOWN) && selectedEffectIndex + 1 < effectChain.effects.size())
					{
						std::swap(effectChain.effects[selectedEffectIndex], effectChain.effects[selectedEffectIndex + 1]);
						selectedEffectIndex++;
					}
				}
				effectChain.unlock();
			}
			framework.endDraw();
		}
		popWindow();
	}
};

std::vector<std::string> scanJsusFxScripts(const char * searchPath, const bool recurse)
{
	std::vector<std::string> result;
	
	auto filenames = listFiles(searchPath, recurse);
	
	JsusFxPathLibrary_Basic pathLibrary(DATA_ROOT);
	JsusFxFileAPI_Basic fileAPI;
	JsusFxGfx gfxAPI;
	
	pathLibrary.addSearchPath("lib");
	
	for (auto & filename : filenames)
	{
		if (String::EndsWith(filename, "-example"))
			continue;
		if (String::EndsWith(filename, "-template"))
			continue;
			
		auto extension = Path::GetExtension(filename, true);
		
		if (extension != "" && extension != "jsfx")
			continue;
		
		JsusFx_Framework jsusFx(pathLibrary);
		
		fileAPI.init(jsusFx.m_vm);
		jsusFx.fileAPI = &fileAPI;
		
		gfxAPI.init(jsusFx.m_vm);
		jsusFx.gfx = &gfxAPI;
		
		if (!jsusFx.readHeader(pathLibrary, filename))
			continue;
		
		if (jsusFx.desc[0] == 0)
			continue;
		
		result.push_back(filename);
	}
	
	std::sort(result.begin(), result.end());
	
	return result;
}

static void testJsusFxList()
{
	if (!framework.init(0, nullptr, 420, 720))
		return;
	
	JsusFx::init();
	
	std::map<std::string, std::vector<std::string>> filenamesByLocation;
	
	filenamesByLocation["Reaper"] = scanJsusFxScripts(SEARCH_PATH_reaper, true);
	filenamesByLocation["ATK"] = scanJsusFxScripts(SEARCH_PATH_ATK, true);
	filenamesByLocation["GeraintLuff"] = scanJsusFxScripts(SEARCH_PATH_geraintluff, false);
	filenamesByLocation["Kawa"] = scanJsusFxScripts(SEARCH_PATH_kawa, true);
	
	std::string activeLocation = "GeraintLuff";
	
	std::vector<JsusFxWindow*> windows;
	
	AudioStream_Vorbis vorbis;
	vorbis.Open("movers.ogg", true);
	
	AudioStream_JsusFxChain audioStream;
	audioStream.init(&vorbis);
	
	AudioOutput_PortAudio audioOutput;
	audioOutput.Initialize(2, SAMPLE_RATE, BUFFER_SIZE);
	audioOutput.Play(&audioStream);
	
	MidiKeyboard midiKeyboard;
	
	JsusFxChainWindow effectChainWindow(audioStream);
	
	bool scrollerIsActive = false;
	float scrollerPosition = 0.f;
	
	for (;;)
	{
		framework.process();
		
		if (keyboard.wentDown(SDLK_ESCAPE))
			framework.quitRequested = true;
		
		if (framework.quitRequested)
			break;
		
		const float dt = framework.timeStep;
		
		for (auto windowItr = windows.begin(); windowItr != windows.end(); )
		{
			auto & window = *windowItr;
			
			window->tick(dt);
			
			if (window->isValid == false)
			{
				audioStream.remove(&window->jsusFx);
				
				delete window;
				window = nullptr;
				
				windowItr = windows.erase(windowItr);
			}
			else
			{
				windowItr++;
			}
		}
		
		effectChainWindow.tick(dt);
		
		framework.beginDraw(0, 0, 0, 0);
		{
			setFont("calibri.ttf");
			
			int x = 0;
			int y = 0;
			
			x += 10;
			y += 10;
			
			gxPushMatrix();
			{
				gxTranslatef(x, y, 0);
				
				audioStream.lock();
				{
					doMidiKeyboard(midiKeyboard, mouse.x - x, mouse.y - y, s_midiBuffer.bytes + s_midiBuffer.numBytes, &s_midiBuffer.numBytes, true, false);
				}
				audioStream.unlock();
				
				doMidiKeyboard(midiKeyboard, mouse.x - x, mouse.y, nullptr, nullptr, false, true);
			}
			gxPopMatrix();
			
			y += 74;
			
			int locationX = 0;
			
			for (auto & locationItr : filenamesByLocation)
			{
				const std::string & location = locationItr.first;
				
				const int x1 = x + locationX;
				const int y1 = y;
				const int x2 = x1 + 80;
				const int y2 = y1 + 20;
				
				const bool isInside = mouse.x >= x1 && mouse.y >= y1 && mouse.x < x2 && mouse.y < y2;
				
				if (isInside && mouse.wentDown(BUTTON_LEFT))
				{
					activeLocation = location;
				}
				
				setColor(colorWhite);
				
				setLumi(isInside ? 200 : 100);
				drawRect(x1, y1, x2, y2);
				setLumi(isInside ? 100 : 200);
				drawText((x1 + x2)/2, (y1 + y2)/2, 16, 0, 0, "%s", location.c_str());
				
				locationX += 90;
			}
			
			y += 24;
			
			auto & filenames = filenamesByLocation[activeLocation];
			
			const int kMaxVisible = 30;
			
			if (filenames.size() > kMaxVisible)
			{
				const float x1 = x;
				const float y1 = y;
				const float x2 = x1 + 400;
				const float y2 = y1 + 16;
				
				const float size = (x2 - x1) - 2*2;
				const float barSize = size * kMaxVisible / float(filenames.size());
				const float scrollPixels = (size - barSize) * scrollerPosition;
				
				const float barX1 = x1 + 2 + scrollPixels;
				const float barY1 = y1 + 2;
				const float barX2 = barX1 + barSize;
				const float barY2 = y2 - 2;
				
				const bool isInside = mouse.x >= x1 && mouse.y >= y1 && mouse.x < x2 && mouse.y < y2;
				
				if (isInside && mouse.wentDown(BUTTON_LEFT))
					scrollerIsActive = true;
				
				if (mouse.wentUp(BUTTON_LEFT))
					scrollerIsActive = false;
				
				if (scrollerIsActive)
				{
					scrollerPosition = clamp((mouse.x - x1) / float(x2 - x1), 0.f, 1.f);
					drawText(0, 0, 20, +1, +1, "SCROLL: %g", scrollerPosition);
				}
				
				hqBegin(HQ_FILLED_ROUNDED_RECTS);
				setLumi(100);
				hqFillRoundedRect(x1, y1, x2, y2, 4);
				setLumi(isInside ? 200 : 160);
				hqFillRoundedRect(barX1, barY1, barX2, barY2, 3);
				hqEnd();
			}
			else
			{
				scrollerPosition = 0.f;
				scrollerIsActive = false;
			}
			
			y += 20;
			
			const int numVisible = filenames.size() < kMaxVisible ? filenames.size() : kMaxVisible;
			
			int index = (filenames.size() - numVisible) * scrollerPosition;
			
			for (int i = 0; i < numVisible; ++i, ++index)
			{
				auto & filename = filenames[index];
				
				setColor(colorWhite);
				
				const bool isInside = mouse.x >= x && mouse.y >= y && mouse.x < x + 400 && mouse.y < y + 14;
				
				if (isInside && mouse.wentDown(BUTTON_LEFT))
				{
					JsusFxWindow * window = new JsusFxWindow(filename.c_str());
					
					if (window->isValid)
					{
						audioStream.add(&window->jsusFx);
					}
					
					windows.push_back(window);
				}
				
				setLumi(isInside ? 200 : 100);
				drawRect(x, y, x + 400, y + 14);
				setLumi(isInside ? 100 : 200);
				drawText(x + 2, y + 14/2, 10, +1, 0, "%s", filename.c_str());
				
				y += 16;
			}
		}
		framework.endDraw();
		
		for (auto & window : windows)
			window->draw();
	}
	
	audioOutput.Stop();
	
	// todo : free effects, windows etc
	
	framework.shutdown();
}

int main(int argc, char * argv[])
{
#if 1
	testJsusFxList();
	exit(0);
#endif
	
	framework.filedrop = true;
	framework.actionHandler = handleAction;
	
	if (!framework.init(0, nullptr, GFX_SX, GFX_SY))
		return -1;
	
	JsusFx::init();
	
	JsusFxPathLibrary_Basic pathLibrary(DATA_ROOT);
	pathLibrary.addSearchPath("lib");
	
	JsusFx_Framework fx(pathLibrary);
	s_fx = &fx;
	
	JsusFxGfx_Framework gfx(fx);
	fx.gfx = &gfx;
	gfx.init(fx.m_vm);
	
	JsusFxFileAPI_Basic fileAPI;
	fx.fileAPI = &fileAPI;
	fileAPI.init(fx.m_vm);
	
	//const char * filename = "3bandpeakfilter";
	//const char * filename = "/Users/thecat/atk-reaper/plugins/FOA/Transform/FocusPressPushZoom";
	//const char * filename = "/Users/thecat/atk-reaper/plugins/FOA/Transform/Direct";
	//const char * filename = "/Users/thecat/jsusfx/scripts/liteon/vumetergfx";
	//const char * filename = "/Users/thecat/jsusfx/scripts/liteon/statevariable";
	//const char * filename = "/Users/thecat/Downloads/JSFX-kawa-master/kawa_XY_Delay.jsfx";
	//const char * filename = "/Users/thecat/Downloads/JSFX-kawa-master/kawa_XY_Chorus.jsfx";
	//const char * filename = "/Users/thecat/Downloads/JSFX-kawa-master/kawa_XY_Flanger.jsfx";
	//const char * filename = "/Users/thecat/geraintluff -jsfx/Spring-Box.jsfx";
	//const char * filename = "/Users/thecat/geraintluff -jsfx/Stereo Alignment Delay.jsfx";
	//const char * filename = "/Users/thecat/atk-reaper/plugins/FOA/Transform/RotateTiltTumble";
	//const char * filename = "/Users/thecat/geraintluff -jsfx/Bad Connection.jsfx";
	//const char * filename = "/Users/thecat/atk-reaper/plugins/FOA/Encode/Quad";
	//const char * filename = "/Users/thecat/atk-reaper/plugins/FOA/Encode/AmbiXToB";
	//const char * filename = "/Users/thecat/atk-reaper/plugins/FOA/Decode/Binaural";
	//const char * filename = "/Users/thecat/atk-reaper/plugins/FOA/Encode/Periphonic3D";
	//const char * filename = "/Users/thecat/atk-reaper/plugins/FOA/Decode/UHJ";
	//const char * filename = "/Users/thecat/geraintluff -jsfx/Warble.jsfx"; // fixme
	//const char * filename = "/Users/thecat/geraintluff -jsfx/Spectrum Matcher.jsfx";
	//const char * filename = "/Users/thecat/geraintluff -jsfx/Smooth Limiter.jsfx";
	//const char * filename = "/Users/thecat/geraintluff -jsfx/Sandwich Amp.jsfx";
	//const char * filename = "/Users/thecat/geraintluff -jsfx/Pulsar.jsfx"; // todo
	//const char * filename = "/Users/thecat/geraintluff -jsfx/Panalysis.jsfx";
	//const char * filename = "/Users/thecat/geraintluff -jsfx/pad-synth.jsfx";
	//const char * filename = "/Users/thecat/geraintluff -jsfx/MIDI Harmony.jsfx"; // todo
	//const char * filename = "/Users/thecat/geraintluff -jsfx/Level Meter.jsfx";
	//const char * filename = "/Users/thecat/geraintluff -jsfx/Learning Sampler.jsfx";
	//const char * filename = "/Users/thecat/geraintluff -jsfx/Humonica.jsfx";
	//const char * filename = "/Users/thecat/geraintluff -jsfx/Hammer And String.jsfx";
	//const char * filename = "/Users/thecat/geraintluff -jsfx/Hammer And Chord.jsfx";
	//const char * filename = "/Users/thecat/geraintluff -jsfx/Echo-Cycles.jsfx";
	//const char * filename = "/Users/thecat/Downloads/Transpire/Transpire";
	const char * filename = "/Users/thecat/Downloads/Surround_Pan_2-JSFX/Surround Pan 2/Surround Pan 2";
	//const char * filename = "/Users/thecat/Downloads/QuadraCom-JSFX/QuadraCom/QuadraCom";
	
	if (!fx.compile(pathLibrary, filename))
	{
		logError("failed to load file: %s", filename);
		return -1;
	}
	
	fx.prepare(SAMPLE_RATE, BUFFER_SIZE);
	
	AudioStream_Vorbis vorbis;
	vorbis.Open("movers.ogg", true);
	
	AudioStream_JsusFx audioStream;
	audioStream.init(&vorbis, &fx);
	s_audioStream = &audioStream;
	
	AudioOutput_PortAudio audioOutput;
	audioOutput.Initialize(2, SAMPLE_RATE, BUFFER_SIZE);
	audioOutput.Play(&audioStream);
	
	MidiKeyboard midiKeyboard;
	
	bool sliderIsActive[JsusFx::kMaxSliders] = { };
	
	for (;;)
	{
		framework.process();
		
		if (keyboard.wentDown(SDLK_ESCAPE))
			framework.quitRequested = true;
		
		if (framework.quitRequested)
			break;

		audioStream.lock();
		{
			doMidiKeyboard(midiKeyboard, mouse.x - 10 - fx.gfx_w, mouse.y - 10, s_midiBuffer.bytes + s_midiBuffer.numBytes, &s_midiBuffer.numBytes, true, false);
		}
		audioStream.unlock();
		
		framework.beginDraw(0, 0, 0, 0);
		{
			setFont("calibri.ttf");
			
			gfx.setup(nullptr, fx.gfx_w, fx.gfx_h, mouse.x, mouse.y, true);
			
			//setDrawRect(0, 0, fx.gfx_w, fx.gfx_h);
			pushFontMode(FONT_SDF);
			setColorClamp(true);
			fx.draw();
			setColorClamp(false);
			popFontMode();
			//clearDrawRect();
			
		#if 0
			{
				// draw the list of used images on screen
				
				int x = 0;
				int index = 0;
				
				for (auto & image : gfx.imageCache.images)
				{
					if (image.isValid)
					{
						setColor(colorWhite);
						gxSetTexture(image.surface->getTexture());
						drawRect(x, 0, x + 40, 40);
						gxSetTexture(0);
						
						drawText(x + 4, 4, 10, +1, +1, "%d", index);
						
						setColor(100, 100, 10);
						drawRectLine(x, 0, x + 40, 40);
						
						x += 40;
					}
					
					index++;
				}
			}
		#endif
			
			const int sy = *gfx.m_gfx_h;
			
			int x = 10;
			int y = sy + 100;
			
			setColor(160, 160, 160);
			drawText(x + 240, y, 18, +1, +1, "JSFX file: %s", Path::GetFileName(filename).c_str());
			drawText(x + 240 + 3, y + 21, 12, +1, +1, "%d ins, %d outs", fx.numInputs, fx.numOutputs);
			
			for (int i = 0; i < JsusFx::kMaxSliders; ++i)
			{
				if (fx.sliders[i].exists && fx.sliders[i].desc[0] != '-')
				{
					gxPushMatrix();
					gxTranslatef(x, y, 0);
					doSlider(fx, fx.sliders[i], mouse.x - x, mouse.y - y, sliderIsActive[i]);
					gxPopMatrix();
					
					y += 16;
				}
			}
			
			gxPushMatrix();
			{
				gxTranslatef(10 + fx.gfx_w, 10, 0);
				
				doMidiKeyboard(midiKeyboard, mouse.x - 10 - fx.gfx_w, mouse.y - 10, nullptr, nullptr, false, true);
			}
			gxPopMatrix();
		}
		framework.endDraw();
	}

	Font("calibri.ttf").saveCache();
	
	framework.shutdown();

	return 0;
}
