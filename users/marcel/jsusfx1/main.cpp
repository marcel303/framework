#include "framework.h"

#include "jsusfx.h"
#include "jsusfx_file.h"
#include "jsusfx_gfx.h"

#include "gfx-framework.h"
#include "jsusfx-framework.h"

#include "Path.h"

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

#define SAMPLE_RATE 44100
#define BUFFER_SIZE 64

#define DATA_ROOT "/Users/thecat/Library/Application Support/REAPER/Data/"

//#define SEARCH_PATH "/Users/thecat/atk-reaper/plugins/"
//#define SEARCH_PATH "/Users/thecat/geraintluff -jsfx/"
#define SEARCH_PATH "/Users/thecat/atk-reaper/plugins/"

const int GFX_SX = 1000;
const int GFX_SY = 720;

static void doSlider(JsusFx & fx, JsusFx_Slider & slider, int x, int y)
{
	const int sx = 200;
	const int sy = 12;
	
	const bool isInside =
		//x >= 0 && x <= sx; // &&
		y >= 0 && y <= sy;
	
	if (isInside && mouse.isDown(BUTTON_LEFT))
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

static bool generateMidiEvent(uint8_t * midi)
{
	static int lastNote = -1;
	static int todo = 0;

	//const bool trigger = (rand() % 1000) == 0;
	const bool trigger = (lastNote == -1) ? (todo-- == 0) : ((rand() % 200) == 0);

	if (trigger) {
		if (lastNote != -1) {
			const uint8_t msg = 0x80;
			const uint8_t note = lastNote;
			const uint8_t value = rand() % 128;
			
			midi[0] = msg;
			midi[1] = note;
			midi[2] = value;
			
			lastNote = -1;
			
			return true;
		} else {
			const uint8_t msg = 0x90;
			const uint8_t note = 32 + (rand() % 32);
			const uint8_t value = rand() % 128;
			
			midi[0] = msg;
			midi[1] = note;
			midi[2] = value;
			
			lastNote = note;
			todo = 100 + (rand() % 400);
			
			return true;
		}
	} else {
		return false;
	}
}

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
	}
	
	void unlock()
	{
		const int r = SDL_UnlockMutex(mutex);
		Assert(r == 0);
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
	AudioStream * source = nullptr;
	
	std::vector<JsusFx*> effects;
	
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
	}
	
	void unlock()
	{
		const int r = SDL_UnlockMutex(mutex);
		Assert(r == 0);
	}
	
	void add(JsusFx * jsusFx)
	{
		lock();
		{
			effects.push_back(jsusFx);
		}
		unlock();
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
		
		memset(outputL, 0, sizeof(outputL));
		memset(outputR, 0, sizeof(outputR));
		
		float * input[2] =
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
			for (auto & jsusFx : effects)
			{
				jsusFx->setMidi(s_midiBuffer.bytes, s_midiBuffer.numBytes);
				
				const float * inputTemp[2] =
				{
					input[0],
					input[1]
				};
				
				if (jsusFx->process(inputTemp, output, numSamples, 2, 2))
				{
					std::swap(input[0], output[0]);
					std::swap(input[1], output[1]);
				}
			}
			
			s_midiBuffer.numBytes = 0;
		}
		unlock();
		
		std::swap(input[0], output[0]);
		std::swap(input[1], output[1]);
		
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

struct JsusFxWindow
{
	Window * window;
	
	JsusFxPathLibrary_Basic pathLibrary;
	
	JsusFx_Framework jsusFx;
	JsusFxFileAPI_Basic fileAPI;
	JsusFxGfx_Framework gfxAPI;
	
	std::string filename;
	
	bool isValid;
	
	JsusFxWindow(const char * _filename)
		: window(nullptr)
		, pathLibrary(DATA_ROOT)
		, jsusFx(pathLibrary)
		, fileAPI()
		, gfxAPI(jsusFx)
		, filename(_filename)
		, isValid(false)
	{
		fileAPI.init(jsusFx.m_vm);
		jsusFx.fileAPI = &fileAPI;
		
		gfxAPI.init(jsusFx.m_vm);
		jsusFx.gfx = &gfxAPI;
		
		if (jsusFx.compile(pathLibrary, filename))
		{
			jsusFx.prepare(SAMPLE_RATE, BUFFER_SIZE);
			
			window = new Window(filename.c_str(), jsusFx.gfx_w, jsusFx.gfx_h, true);
			
			isValid = true;
		}
	}
	
	~JsusFxWindow()
	{
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
			framework.beginDraw(0, 0, 0, 0);
			{
				gfxAPI.setup(nullptr, window->getWidth(), window->getHeight(), mouse.x, mouse.y, true);
				
				pushFontMode(FONT_SDF);
				setColorClamp(true);
				{
					jsusFx.draw();
				}
				setColorClamp(false);
				popFontMode();
				
				int x = 10;
				int y = 10;
				
				setColor(160, 160, 160);
				drawText(x, y, 18, +1, +1, "JSFX file: %s", Path::GetFileName(filename).c_str());
				y += 20;
				
				setColor(160, 160, 160);
				drawText(x + 3, y + 21, 12, +1, +1, "%d ins, %d outs", jsusFx.numInputs, jsusFx.numOutputs);
				y += 14;
				
				for (auto & slider : jsusFx.sliders)
				{
					if (slider.exists && slider.desc[0] != '-')
					{
						gxPushMatrix();
						gxTranslatef(x, y, 0);
						doSlider(jsusFx, slider, mouse.x - x, mouse.y - y);
						gxPopMatrix();
						
						y += 16;
					}
				}
				
				x += 300;
				
				if (x > window->getWidth() || y > window->getHeight())
				{
					window->setSize(x, y);
				}
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
	
	for (auto & filename : filenames)
	{
		auto extension = Path::GetExtension(filename, true);
		
		if (extension != "" && extension != "jsfx")
			continue;
		
		JsusFx_Framework jsusFx(pathLibrary);
		
		fileAPI.init(jsusFx.m_vm);
		jsusFx.fileAPI = &fileAPI;
		
		gfxAPI.init(jsusFx.m_vm);
		jsusFx.gfx = &gfxAPI;
		
		if (!jsusFx.compile(pathLibrary, filename))
			continue;
		
		if (jsusFx.desc[0] == 0)
			continue;
		
		result.push_back(filename);
	}
	
	return result;
}

static void testJsusFxList()
{
	if (!framework.init(0, nullptr, 420, 640))
		return;
	
	JsusFx::init();
	
	//auto filenames = scanJsusFxScripts(SEARCH_PATH, false);
	auto filenames = scanJsusFxScripts(SEARCH_PATH, true);
	
	std::vector<JsusFxWindow*> windows;
	
	AudioStream_Vorbis vorbis;
	vorbis.Open("movers.ogg", true);
	
	AudioStream_JsusFxChain audioStream;
	audioStream.init(&vorbis);
	
	AudioOutput_PortAudio audioOutput;
	audioOutput.Initialize(2, SAMPLE_RATE, BUFFER_SIZE);
	audioOutput.Play(&audioStream);
	
	MidiKeyboard midiKeyboard;
	
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
				delete window;
				window = nullptr;
				
				windowItr = windows.erase(windowItr);
			}
			else
			{
				windowItr++;
			}
		}
		
		framework.beginDraw(0, 0, 0, 0);
		{
			setFont("calibri.ttf");
			
			int x = 0;
			int y = 0;
			
			x += 10;
			y += 10;
			
			gxPushMatrix();
			{
				audioStream.lock();
				{
					doMidiKeyboard(midiKeyboard, mouse.x - x, mouse.y - y, s_midiBuffer.bytes + s_midiBuffer.numBytes, &s_midiBuffer.numBytes, true, false);
				}
				audioStream.unlock();
				
				doMidiKeyboard(midiKeyboard, mouse.x - x, mouse.y, nullptr, nullptr, false, true);
			}
			gxPopMatrix();
			
			y += 70;
			
			for (auto & filename : filenames)
			{
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
	
	double t = 0.0;
	double ts = 0.0;
	
	MidiKeyboard midiKeyboard;
	
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
			
		#if 0
			for (int a = 0; a < 10; ++a)
			{
				float ind[2][BUFFER_SIZE];
				float * in[2] = { ind[0], ind[1] };
				float outd[2][BUFFER_SIZE];
				float * out[2] = { outd[0], outd[1] };
				
				for (int i = 0; i < BUFFER_SIZE; ++i)
				{
					in[0][i] = cos(t) * 0.2;
					in[1][i] = sin(t) * 0.3 + random(-0.1, +0.1);
					
					double freq = lerp<double>(100.0, 1000.0, (sin(ts) + 1.0) / 2.0);
					t += 2.0 * M_PI * freq / double(SAMPLE_RATE);
					ts += 2.0 * M_PI * 0.2 / double(SAMPLE_RATE);
				}
				
				fx.process(in, out, 64);
			}
		#endif
			
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
			
			const int sx = *gfx.m_gfx_w;
			const int sy = *gfx.m_gfx_h;
			
			int x = 10;
			int y = sy + 100;
			
			setColor(160, 160, 160);
			drawText(x + 240, y, 18, +1, +1, "JSFX file: %s", Path::GetFileName(filename).c_str());
			drawText(x + 240 + 3, y + 21, 12, +1, +1, "%d ins, %d outs", fx.numInputs, fx.numOutputs);
			
			for (int i = 0; i < 64; ++i)
			{
				if (fx.sliders[i].exists && fx.sliders[i].desc[0] != '-')
				{
					gxPushMatrix();
					gxTranslatef(x, y, 0);
					doSlider(fx, fx.sliders[i], mouse.x - x, mouse.y - y);
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
