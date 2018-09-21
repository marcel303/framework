#include "framework.h"

#include "jsusfx.h"
#include "jsusfx_file.h"
#include "jsusfx_gfx.h"
#include "jsusfx_serialize.h"

#include "gfx-framework.h"
#include "jsusfx-framework.h"

#include "audioIO.h"

#include "BoxAtlas.h"
#include "Path.h"
#include "StringEx.h"
#include "Timer.h"

#include "tinyxml2.h"
#include "tinyxml2_helpers.h"

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

+ add octave select to on-screen MIDI keyboard
+ bring JSFX window to top when selecting it in the effect chain window
+ draw a cross when an effect is active in the effect chain list
+ show CPU-usage per effect (perhaps in the effect chain list?)
- show effect stats when hovering over an effect in the effect chain list like:
	+ number of I/O pins,
	- description,
	- sliders
+ don't draw JSFX UI when a window is minimized
+ add an option to hide to JSFX UI from the effect chain list
+ perhaps hide should be the default behavior of the close button, instead of closing and removing it from the effect chain?
+ add an effect search filter box
- add button to connect/disconnect midi
- add option to organize windows (using box atlas?)
+ make octave buttons round
- add ability to load/save effect chains
	- add load/save to xml
	- add load/select window for saved effect chains
+ add option to enable live audio input

*/

#define SAMPLE_RATE 44100
#define BUFFER_SIZE 64

#define DATA_ROOT "/Users/thecat/Library/Application Support/REAPER/Data/"

#define SEARCH_PATH_reaper "/Users/thecat/Library/Application Support/REAPER/Effects/"
#define SEARCH_PATH_geraintluff "/Users/thecat/geraintluff -jsfx/"
#define SEARCH_PATH_ATK "/Users/thecat/atk-reaper/plugins/"
#define SEARCH_PATH_kawa "/Users/thecat/Downloads/JsusFx - Kawa/"

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
		bool isDown;
	};
	
	Key keys[kNumKeys];
	
	int octave = 4;
	
	MidiKeyboard()
	{
		for (int i = 0; i < kNumKeys; ++i)
		{
			auto & key = keys[i];
			
			key.isDown = false;
		}
	}
	
	int getNote(const int keyIndex) const
	{
		return octave * 8 + keyIndex;
	}
	
	void changeOctave(const int direction)
	{
		octave = clamp(octave + direction, 0, 10);
		
		for (int i = 0; i < kNumKeys; ++i)
		{
			auto & key = keys[i];
			
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

void doMidiKeyboard(MidiKeyboard & kb, const int mouseX, const int mouseY, uint8_t * midi, int * midiSize, const bool doTick, const bool doDraw, int & out_sx, int & out_sy)
{
	const int keySx = 16;
	const int keySy = 64;
	const int octaveSx = 24;
	const int octaveSy = 24;

	const int hoverIndex = mouseY >= 0 && mouseY <= keySy ? mouseX / keySx : -1;
	const float velocity = clamp(mouseY / float(keySy), 0.f, 1.f);
	
	const int octaveX = keySx * MidiKeyboard::kNumKeys + 4;
	const int octaveY = 4;
	const int octaveHoverIndex = mouseX >= octaveX && mouseX <= octaveX + octaveSx ? (mouseY - octaveY) / octaveSy : -1;
	
	const int sx = std::max(keySx * MidiKeyboard::kNumKeys, octaveX + octaveSx);
	const int sy = std::max(keySy, octaveSy * 2);
	
	out_sx = sx;
	out_sy = sy;
	
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
					
					writeMidi(midi, *midiSize, MIDI_ON, kb.getNote(i), velocity * 127);
				}
				else
				{
					writeMidi(midi, *midiSize, MIDI_OFF, kb.getNote(i), velocity * 127);
					
					key.isDown = false;
				}
			}
		}
		
		if (mouse.wentDown(BUTTON_LEFT))
		{
			if (octaveHoverIndex == 0)
				kb.changeOctave(+1);
			if (octaveHoverIndex == 1)
				kb.changeOctave(-1);
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
		
		{
			gxPushMatrix();
			{
				gxTranslatef(octaveX, octaveY, 0);
				
				//setLumi(200);
				//drawText(octaveSx + 4, 4, 12, +1, +1, "octave: %d", kb.octave);
				
				setColor(0 == octaveHoverIndex ? colorkeyHover : colorKey);
				hqSetGradient(GRADIENT_LINEAR, Mat4x4(true).RotateZ(M_PI/2.f).Scale(1.f, 1.f / (octaveSy * 2), 1.f).Translate(-octaveX, -octaveY, 0), Color(140, 180, 220), colorWhite, COLOR_MUL);
				hqBegin(HQ_FILLED_ROUNDED_RECTS);
				hqFillRoundedRect(0, 0, octaveSx, octaveSy, 2.f);
				hqEnd();
				hqClearGradient();
				
				setLumi(40);
				hqBegin(HQ_FILLED_TRIANGLES);
				hqFillTriangle(octaveSx/2, 6, octaveSx - 6, octaveSy - 6, 6, octaveSy - 6);
				hqEnd();
				
				gxTranslatef(0, octaveSy, 0);
				
				setColor(1 == octaveHoverIndex ? colorkeyHover : colorKey);
				hqSetGradient(GRADIENT_LINEAR, Mat4x4(true).RotateZ(M_PI/2.f).Scale(1.f, 1.f / (octaveSy * 2), 1.f).Translate(-octaveX, -octaveY, 0), Color(140, 180, 220), colorWhite, COLOR_MUL);
				hqBegin(HQ_FILLED_ROUNDED_RECTS);
				hqFillRoundedRect(0, 0, octaveSx, octaveSy, 2.f);
				hqEnd();
				hqClearGradient();
				
				setLumi(40);
				hqBegin(HQ_FILLED_TRIANGLES);
				hqFillTriangle(octaveSx/2, octaveSy - 6, octaveSx - 6, 6, 6, 6);
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
	
	uint64_t cpuTime = 0;
	
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
			uint64_t time1 = g_TimerRT.TimeUS_get();
			
			fx->setMidi(s_midiBuffer.bytes, s_midiBuffer.numBytes);
			
			fx->process(input, output, numSamples, 2, 2);
			
			s_midiBuffer.numBytes = 0;
			
			uint64_t time2 = g_TimerRT.TimeUS_get();
			
			cpuTime = time2 - time1;
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
			s_fx->compile(pathLibrary, filename,
				JsusFx::kCompileFlag_CompileGraphicsSection |
				JsusFx::kCompileFlag_CompileSerializeSection);
			
			s_fx->prepare(SAMPLE_RATE, BUFFER_SIZE);
		}
		s_audioStream->unlock();
	}
}

//

struct JsusFxElem
{
	int id = 0;
	bool isPassthrough = false;
	std::string filename;

	JsusFx_Framework jsusFx;
	JsusFxFileAPI_Basic fileAPI;
	JsusFxGfx_Framework gfxAPI;
	
	bool isValid = false;
	
	uint64_t cpuTime = 0;
	
	JsusFxElem(JsusFxPathLibrary & pathLibrary)
		: jsusFx(pathLibrary)
		, fileAPI()
		, gfxAPI(jsusFx)
	{
	}
	
	void init()
	{
		fileAPI.init(jsusFx.m_vm);
		jsusFx.fileAPI = &fileAPI;
		
		gfxAPI.init(jsusFx.m_vm);
		jsusFx.gfx = &gfxAPI;
		
		if (jsusFx.compile(jsusFx.pathLibrary, filename,
			JsusFx::kCompileFlag_CompileGraphicsSection |
			JsusFx::kCompileFlag_CompileSerializeSection))
		{
			jsusFx.prepare(SAMPLE_RATE, BUFFER_SIZE);
			
			isValid = true;
		}
	}
};

struct JsusFxChain
{
	JsusFxPathLibrary_Basic pathLibrary;
	
	std::vector<JsusFxElem*> effects;
	
	int nextEffectId = 1;
	
	JsusFxChain(const char * dataRoot)
		: pathLibrary(dataRoot)
	{
		pathLibrary.addSearchPath("lib");
	}
	
	void add(JsusFxElem * elem)
	{
		elem->id = nextEffectId++;
		
		effects.push_back(elem);
	}
	
	void remove(JsusFxElem * elem)
	{
		auto i = std::find(effects.begin(), effects.end(), elem);
		
		Assert(i != effects.end());
		if (i != effects.end())
		{
			effects.erase(i);
		}
	}
	
	void loadXml(tinyxml2::XMLElement * xml_effectChain)
	{
		Assert(effects.empty());
		Assert(nextEffectId == 1);
		
		for (auto xml_effect = xml_effectChain->FirstChildElement("effect"); xml_effect != nullptr; xml_effect = xml_effect->NextSiblingElement("effect"))
		{
			JsusFxElem * elem = new JsusFxElem(pathLibrary);
			
			elem->id = intAttrib(xml_effect, "id", nextEffectId);
			
			elem->isPassthrough = boolAttrib(xml_effect, "passthrough", false);
			elem->filename = stringAttrib(xml_effect, "filename", "");
			
			elem->init();
			
			//
			
			JsusFxSerializationData serializationData;
			
			auto xml_sliders = xml_effect->FirstChildElement("sliders");
			
			if (xml_sliders != nullptr)
			{
				for (auto xml_slider = xml_sliders->FirstChildElement("slider"); xml_slider != nullptr; xml_slider = xml_slider->NextSiblingElement("slider"))
				{
					JsusFxSerializationData::Slider slider;
					
					slider.index = intAttrib(xml_slider, "index", -1);
					slider.value = floatAttrib(xml_slider, "value", 0.f);
					
					serializationData.sliders.push_back(slider);
				}
			}
			
			auto xml_vars = xml_effect->FirstChildElement("vars");
			
			if (xml_vars != nullptr)
			{
				for (auto xml_var = xml_vars->FirstChildElement("var"); xml_var != nullptr; xml_var = xml_var->NextSiblingElement("var"))
				{
					const float value = floatAttrib(xml_var, "value", 0.f);
					
					serializationData.vars.push_back(value);
				}
			}
			
			JsusFxSerializer_Basic serializer(serializationData);
			
			elem->jsusFx.serialize(serializer, false);
			
			//
			
			effects.push_back(elem);
			
			nextEffectId = std::max(nextEffectId, elem->id) + 1;
		}
	}

	void saveXml(tinyxml2::XMLPrinter & p)
	{
		for (auto & effect : effects)
		{
			p.OpenElement("effect");
			{
				p.PushAttribute("id", effect->id);
				p.PushAttribute("passthrough", effect->isPassthrough);
				p.PushAttribute("filename", effect->filename.c_str());
				
				// serialize jsfx data
				
				JsusFxSerializationData serializationData;
				
				JsusFxSerializer_Basic serializer(serializationData);
				
				if (effect->jsusFx.serialize(serializer, true))
				{
					if (!serializationData.sliders.empty())
					{
						p.OpenElement("sliders");
						{
							for (auto & slider : serializationData.sliders)
							{
								p.OpenElement("slider");
								{
									p.PushAttribute("index", slider.index);
									p.PushAttribute("value", slider.value);
								}
								p.CloseElement();
							}
						}
						p.CloseElement();
					}
					
					if (!serializationData.vars.empty())
					{
						p.OpenElement("vars");
						{
							for (auto & var : serializationData.vars)
							{
								p.OpenElement("var");
								{
									p.PushAttribute("value", var);
								}
								p.CloseElement();
							}
						}
						p.CloseElement();
					}
				}
			}
			p.CloseElement();
		}
	}
};

struct AudioStream_JsusFxChain : AudioStream, AudioIOCallback
{
	AudioStream * source = nullptr;
	
	JsusFxChain & effectChain;
	
	SDL_mutex * mutex = nullptr;
	
	Limiter limiter;
	
	std::atomic<bool> enableInput;
	
	AudioStream_JsusFxChain(JsusFxChain & _effectChain)
		: effectChain(_effectChain)
	{
		enableInput = false;
	}
	
	virtual ~AudioStream_JsusFxChain() override
	{
		if (mutex != nullptr)
		{
			SDL_DestroyMutex(mutex);
			mutex = nullptr;
		}
	}
	
	void init()
	{
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
		float * samples = (float*)alloca(numSamples * 2 * sizeof(float));
		
		const int numSourceSamples = source->Provide(numSamples, buffer);
		
		for (int i = 0; i < numSourceSamples; ++i)
		{
			samples[i * 2 + 0] = buffer[i].channel[0] / float(1 << 15);
			samples[i * 2 + 1] = buffer[i].channel[1] / float(1 << 15);
		}
		
		for (int i = numSourceSamples; i < numSamples; ++i)
		{
			samples[i * 2 + 0] = 0.f;
			samples[i * 2 + 1] = 0.f;
		}
		
		process(samples, numSamples);
		
		for (int i = 0; i < numSamples; ++i)
		{
			const float valueL = samples[i * 2 + 0];
			const float valueR = samples[i * 2 + 1];
			
			buffer[i].channel[0] = valueL * float((1 << 15) - 2);
			buffer[i].channel[1] = valueR * float((1 << 15) - 2);
		}
		
		return numSamples;
	}
	
	virtual void audioCallback(const float * input, float * output, const int numSamples) override
	{
		if (enableInput)
		{
			for (int i = 0; i < numSamples; ++i)
			{
				output[i * 2 + 0] = input[i];
				output[i * 2 + 1] = input[i];
			}
		}
		else
		{
			memset(output, 0, numSamples * 2 * sizeof(float));
		}
		
		process(output, numSamples);
	}
	
	void process(float * __restrict samples, const int numSamples)
	{
		Assert(numSamples == BUFFER_SIZE);
		
		const int kNumBuffers = 4;
		float bufferData[2][kNumBuffers][BUFFER_SIZE];
		memset(bufferData, 0, sizeof(bufferData));
		
		int inputIndex = 0;
		
		for (int i = 0; i < numSamples; ++i)
		{
			bufferData[inputIndex][0][i] = samples[i * 2 + 0];
			bufferData[inputIndex][1][i] = samples[i * 2 + 1];
		}
		
		lock();
		{
			for (auto & effect : effectChain.effects)
			{
				if (effect->isPassthrough)
				{
					effect->cpuTime = 0;
					continue;
				}
				
				auto & jsusFx = effect->jsusFx;
				
				const uint64_t time1 = g_TimerRT.TimeUS_get();
				
				jsusFx.setMidi(s_midiBuffer.bytes, s_midiBuffer.numBytes);
				
				const float * input[kNumBuffers];
				float * output[kNumBuffers];
				
				for (int i = 0; i < kNumBuffers; ++i)
				{
					input[i] = bufferData[inputIndex][i];
					output[i] = bufferData[1 - inputIndex][i];
				}
				
				if (jsusFx.process(input, output, numSamples, kNumBuffers, kNumBuffers))
				{
					inputIndex = 1 - inputIndex;
				}
				
				const uint64_t time2 = g_TimerRT.TimeUS_get();
				
				effect->cpuTime = time2 - time1;
			}
			
			s_midiBuffer.numBytes = 0;
		}
		unlock();
		
		inputIndex = 1 - inputIndex;
		
		for (int i = 0; i < numSamples; ++i)
		{
			const float valueL = limiter.next(bufferData[1 - inputIndex][0][i]);
			const float valueR = limiter.next(bufferData[1 - inputIndex][1][i]);
			
			samples[i * 2 + 0] = valueL;
			samples[i * 2 + 1] = valueR;
		}
	}
};

struct JsusFxWindow
{
	Window * window;
#if RENDER_TO_SURFACE
	Surface * surface;
#endif
	
	JsusFxElem * effectElem;
	
	bool cleanup;
	
	bool sliderIsActive[JsusFx::kMaxSliders];
	
	JsusFxWindow(JsusFxElem * _effectElem)
		: window(nullptr)
	#if RENDER_TO_SURFACE
		, surface(nullptr)
	#endif
		, effectElem(_effectElem)
		, cleanup(false)
	{
		memset(sliderIsActive, 0, sizeof(sliderIsActive));
		
		const std::string caption = String::FormatC("%s (%d ins, %d outs)",
			Path::GetFileName(effectElem->filename).c_str(),
			effectElem->jsusFx.numInputs,
			effectElem->jsusFx.numOutputs);
		
		window = new Window(caption.c_str(), effectElem->jsusFx.gfx_w, effectElem->jsusFx.gfx_h, true);
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
		if (!effectElem->isValid)
			return;
		
		if (window->getQuitRequested())
		{
			window->hide();
		}
	}
	
	void draw()
	{
		if (!effectElem->isValid)
			return;
		
		if (window->isHidden())
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

				effectElem->gfxAPI.setup(surface, window->getWidth(), window->getHeight(), mouse.x, mouse.y, true);
				
				pushFontMode(FONT_SDF);
				setColorClamp(true);
				{
					effectElem->jsusFx.draw();
				}
				setColorClamp(false);
				popFontMode();
				
			#if RENDER_TO_SURFACE
				surface->blit(BLEND_OPAQUE);
			#endif
				
				int x = 10;
				int y = 10;
				
				int sliderIndex = 0;
				
				for (auto & slider : effectElem->jsusFx.sliders)
				{
					if (slider.exists && slider.desc[0] != '-')
					{
						gxPushMatrix();
						gxTranslatef(x, y, 0);
						doSlider(effectElem->jsusFx, slider, mouse.x - x, mouse.y - y, sliderIsActive[sliderIndex]);
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
	
	Surface * surface;
	
	JsusFxChain & effectChain;
	
	AudioStream_JsusFxChain & effectChainAudioStream;
	
	std::vector<JsusFxWindow*> & windows;
	
	int selectedEffectIndex;
	
	JsusFxChainWindow(JsusFxChain & _effectChain, AudioStream_JsusFxChain & _effectChainAudioStream, std::vector<JsusFxWindow*> & _windows)
		: window("Effect Chain", 300, 300, true)
		, surface(nullptr)
		, effectChain(_effectChain)
		, effectChainAudioStream(_effectChainAudioStream)
		, windows(_windows)
		, selectedEffectIndex(0)
	{
	}
	
	~JsusFxChainWindow()
	{
		delete surface;
		surface = nullptr;
	}
	
	void tick(const float dt)
	{
		pushWindow(window);
		{
			if (surface == nullptr || surface->getWidth() != window.getWidth() || surface->getHeight() != window.getHeight())
			{
				delete surface;
				surface = nullptr;
				
				surface = new Surface(window.getWidth(), window.getHeight(), false);
			}
			
			if (!framework.events.empty())
			{
				pushSurface(surface);
				surface->clear(50, 50, 50, 0);
				
				setFont("calibri.ttf");
				setColor(colorWhite);
				
				int x = 10;
				int y = 10;
				
				int effectIndex = 0;
				
				for (auto & effect : effectChain.effects)
				{
					JsusFxWindow * effectWindow = nullptr;
					
					for (JsusFxWindow * window : windows)
					{
						if (window->effectElem == effect)
							effectWindow = window;
					}
					
					Assert(effectWindow != nullptr);
					
					if (keyboard.wentDown(SDLK_SPACE))
						if (effectIndex == selectedEffectIndex)
							effect->isPassthrough = !effect->isPassthrough;
					
					if (keyboard.wentDown(SDLK_DELETE) || keyboard.wentDown(SDLK_BACKSPACE))
						if (effectIndex == selectedEffectIndex)
							if (effectWindow != nullptr)
								effectWindow->cleanup = true;
				
					if (effectWindow != nullptr)
					{
						const int x1 = x;
						const int y1 = y;
						const int x2 = x + 36;
						const int y2 = y + 16;
						
						const bool isInside =
							mouse.x >= x1 &&
							mouse.y >= y1 &&
							mouse.x <= x2 &&
							mouse.y <= y2;
						
						hqBegin(HQ_FILLED_ROUNDED_RECTS);
						setLumi(isInside ? 100 : 40);
						hqFillRoundedRect(x1, y1, x2, y2, 4.f);
						hqEnd();
						
						setLumi(200);
						if (effectWindow->window->isHidden())
							drawText((x1 + x2)/2, (y1 + y2)/2, 12, 0.f, 0.f, "Show");
						else
							drawText((x1 + x2)/2, (y1 + y2)/2, 12, 0.f, 0.f, "Hide");
						
						if (isInside && mouse.wentDown(BUTTON_LEFT))
						{
							if (effectWindow->window->isHidden())
							{
								effectWindow->window->show();
								window.raise();
							}
							else
								effectWindow->window->hide();
						}
					}
					
					{
						const int x1 = x + 40;
						const int y1 = y;
						const int x2 = x1 + 16;
						const int y2 = y1 + 16;
						
						const bool isInside =
							mouse.x >= x1 &&
							mouse.y >= y1 &&
							mouse.x <= x2 &&
							mouse.y <= y2;
						
						if (isInside && mouse.wentDown(BUTTON_LEFT))
							effect->isPassthrough = !effect->isPassthrough;
						
						setLumi(effect->isPassthrough ? 0 : 200);
						drawRect(x1, y1, x2, y2);
						setLumi(100);
						if (!effect->isPassthrough)
						{
							// draw a cross when checked
							hqBegin(HQ_LINES);
							hqLine(x1 + 3, y1 + 3, 1.1f, x2 - 3, y2 - 3, 1.1f);
							hqLine(x1 + 3, y2 - 3, 1.1f, x2 - 3, y1 + 3, 1.1f);
							hqEnd();
						}
						drawRectLine(x1, y1, x2, y2);
					}
					
					{
						const int x1 = x + 60;
						const int y1 = y;
						const int x2 = window.getWidth();
						const int y2 = y + 16;
						
						const bool isInside =
							mouse.x >= x1 &&
							mouse.y >= y1 &&
							mouse.x <= x2 &&
							mouse.y <= y2;
						
						if (isInside && mouse.wentDown(BUTTON_LEFT))
						{
							selectedEffectIndex = effectIndex;
							
							if (effectWindow != nullptr)
							{
								effectWindow->window->raise();
							
								window.raise();
							}
						}
						
						if (effectIndex == selectedEffectIndex)
						{
							setLumi(0);
							drawRect(x1, y1, x2, y2);
						}
						
						setLumi(200);
						drawText(x1, (y1 + y2)/2, 14, +1, 0, "%s", effect->jsusFx.desc);
						y += 20;
					}
					
					effectIndex++;
				}
				
				effectChainAudioStream.lock();
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
				effectChainAudioStream.unlock();
				
				if (selectedEffectIndex >= 0 && selectedEffectIndex < effectChain.effects.size())
				{
					gxPushMatrix();
					{
						gxTranslatef(0, window.getHeight() - 70, 0);
						
						auto & effect = effectChain.effects[selectedEffectIndex];
						
						hqBegin(HQ_FILLED_ROUNDED_RECTS);
						{
							setLumi(200);
							hqFillRoundedRect(0, 0, window.getWidth(), 70, 4.f);
						}
						hqEnd();
						
						setLumi(40);
						
						int x = 7;
						int y = 7;
						
						drawText(x, y, 12, +1, +1, "file: %s", Path::GetFileName(effect->filename).c_str());
						y += 16;
						
						drawText(x, y, 12, +1, +1, "pins: %d in, %d out", effect->jsusFx.numInputs, effect->jsusFx.numOutputs);
						y += 14;
						
						drawText(x, y, 12, +1, +1, "CPU time: %.2f%%", effect->cpuTime * 100 * 44100 / BUFFER_SIZE / 1000000.f);
						y += 14;
						
						
						
					#if 0
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
					#endif
					}
					gxPopMatrix();
				}
				
				popSurface();
			}
			
			framework.beginDraw(0, 0, 0, 0);
			{
				surface->blit(BLEND_OPAQUE);
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
	
	JsusFxChain effectChain(DATA_ROOT);
	
	std::vector<JsusFxWindow*> windows;
	
	//AudioStream_Vorbis vorbis;
	//vorbis.Open("movers.ogg", true);
	
	AudioStream_JsusFxChain audioStream(effectChain);
	audioStream.init();
	
	MidiKeyboard midiKeyboard;
	
	{
		const char * filename = "effectChain.xml";
		
		tinyxml2::XMLDocument d;

		if (d.LoadFile(filename) == tinyxml2::XML_SUCCESS)
		{
			auto xml_effectChain = d.FirstChildElement("effectChain");
			
			if (xml_effectChain != nullptr)
			{
				effectChain.loadXml(xml_effectChain);
			}
			
			//
			
			for (auto & effect : effectChain.effects)
			{
				JsusFxWindow * window = new JsusFxWindow(effect);
				
				windows.push_back(window);
			}
			
			//
			
			auto xml_editor = d.FirstChildElement("editor");
			
			if (xml_editor != nullptr)
			{
				auto xml_midiKeyboard = xml_editor->FirstChildElement("midiKeyboard");
				
				if (xml_midiKeyboard != nullptr)
				{
					midiKeyboard.octave = intAttrib(xml_midiKeyboard, "octave", midiKeyboard.octave);
				}
				
				auto xml_audioInput = xml_editor->FirstChildElement("audioInput");
				
				if (xml_audioInput != nullptr)
				{
					audioStream.enableInput = boolAttrib(xml_audioInput, "enabled", audioStream.enableInput);
				}
				
				auto xml_effectChain = xml_editor->FirstChildElement("effectChain");
				
				if (xml_effectChain != nullptr)
				{
					for (auto xml_effect = xml_effectChain->FirstChildElement("effect"); xml_effect != nullptr; xml_effect = xml_effect->NextSiblingElement("effect"))
					{
						const int id = intAttrib(xml_effect, "id", 0);
						
						if (id != 0)
						{
							auto windowItr = std::find_if(windows.begin(), windows.end(), [&](JsusFxWindow * w) -> bool { return w->effectElem->id == id; });
							
							if (windowItr != windows.end())
							{
								JsusFxWindow * window = *windowItr;
								
								const int windowX = intAttrib(xml_effect, "window_x", 0);
								const int windowY = intAttrib(xml_effect, "window_y", 0);
								const int windowSx = intAttrib(xml_effect, "window_sx", window->window->getWidth());
								const int windowSy = intAttrib(xml_effect, "window_sy", window->window->getHeight());
								const bool windowIsVisible = boolAttrib(xml_effect, "window_visible", true);
								
								if (!windowIsVisible)
									window->window->hide();
								
								window->window->setPosition(windowX, windowY);
								window->window->setSize(windowSx, windowSy);
							}
						}
					}
				}
			}
		}
	}
	
#if 1
	AudioIO audioIO;
	audioIO.Initialize(2, 1, SAMPLE_RATE, BUFFER_SIZE);
	audioIO.Play(&audioStream);
#else
	AudioOutput_PortAudio audioOutput;
	audioOutput.Initialize(2, SAMPLE_RATE, BUFFER_SIZE);
	audioOutput.Play(&audioStream);
#endif
	
	JsusFxChainWindow effectChainWindow(effectChain, audioStream, windows);
	
	bool scrollerIsActive = false;
	float scrollerPosition = 0.f;
	
	std::string filenameFilter;
	
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
			
			if (window->cleanup)
			{
				audioStream.lock();
				{
					effectChain.remove(window->effectElem);
				}
				audioStream.unlock();
				
				delete window->effectElem;
				window->effectElem = nullptr;
				
				delete window;
				window = nullptr;
				
				windowItr = windows.erase(windowItr);
			}
			else
			{
				windowItr++;
			}
		}
		
	#if 0 // todo : keep or remove ?
		if (mouse.wentDown(BUTTON_RIGHT))
		{
			BoxAtlas atlas;
			
			atlas.init(1280, 10000); // todo : desktop size
			
			struct Elem
			{
				Window * window = nullptr;
				BoxAtlasElem * atlasElem = nullptr;
			};
			
			std::vector<Elem> elems;
			Elem elem;
			elem.window = &effectChainWindow.window;
			elems.push_back(elem);
			
			for (auto & window : windows)
			{
				Elem elem;
				elem.window = window->window;
				elems.push_back(elem);
			}
			
			for (auto & elem : elems)
				elem.atlasElem = atlas.tryAlloc(elem.window->getWidth(), elem.window->getHeight());
			
			atlas.optimize();
			
			for (auto & elem : elems)
				if (elem.atlasElem != nullptr)
					elem.window->setPosition(elem.atlasElem->x, elem.atlasElem->y);
		}
	#endif
		
		effectChainWindow.tick(dt);
		
		framework.beginDraw(0, 0, 0, 0);
		{
			setFont("calibri.ttf");
			
			int x = 0;
			int y = 0;
			
			x += 10;
			y += 10;
			
			int midiSx;
			int midiSy;
			
			gxPushMatrix();
			{
				gxTranslatef(x, y, 0);
				
				audioStream.lock();
				{
					doMidiKeyboard(midiKeyboard, mouse.x - x, mouse.y - y, s_midiBuffer.bytes + s_midiBuffer.numBytes, &s_midiBuffer.numBytes, true, false, midiSx, midiSy);
				}
				audioStream.unlock();
				
				doMidiKeyboard(midiKeyboard, mouse.x - x, mouse.y - y, nullptr, nullptr, false, true, midiSx, midiSy);
			}
			gxPopMatrix();
			
			gxPushMatrix();
			{
				const int old_x = x;
				
				x += midiSx;
				x += 10;
				
				gxTranslatef(x, y, 0);
				
				const int x1 = 0;
				const int y1 = 0;
				const int x2 = 20;
				const int y2 = 20;
				
				const int mx = mouse.x - x;
				const int my = mouse.y - y;
				
				if (mouse.wentDown(BUTTON_LEFT) &&
					mx >= x1 &&
					my >= y1 &&
					mx <= x2 &&
					my <= y2)
				{
					audioStream.enableInput = !audioStream.enableInput;
				}
				
				setLumi(audioStream.enableInput ? 200 : 0);
				drawRect(x1, y1, x2, y2);
				setLumi(100);
				if (audioStream.enableInput)
				{
					// draw a cross when checked
					hqBegin(HQ_LINES);
					hqLine(x1 + 3, y1 + 3, 1.1f, x2 - 3, y2 - 3, 1.1f);
					hqLine(x1 + 3, y2 - 3, 1.1f, x2 - 3, y1 + 3, 1.1f);
					hqEnd();
				}
				drawRectLine(x1, y1, x2, y2);
				
				x = old_x;
			}
			gxPopMatrix();
			
			y += midiSy;
			y += 10;
			
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
			
			// search filter box
			
			{
				setLumi(filenameFilter.empty() ? 40 : 60);
				hqBegin(HQ_FILLED_ROUNDED_RECTS);
				hqFillRoundedRect(10, y, 10 + 400, y + 20, 4.f);
				hqEnd();
				setLumi(200);
				drawText(10 + 4, y + 10, 12, +1, 0, "%s", filenameFilter.c_str());
				for (auto & e : framework.events)
				{
					if (e.type == SDL_KEYDOWN)
					{
						if (e.key.keysym.sym >= SDLK_a && e.key.keysym.sym <= SDLK_z)
							filenameFilter.push_back(e.key.keysym.sym - SDLK_a + 'a');
					}
				}
				
				y += 20;
			}
			
			if (keyboard.wentDown(SDLK_BACKSPACE))
				filenameFilter.clear();
			
			// apply search filter
			
			auto & allFilenames = filenamesByLocation[activeLocation];
			
			std::vector<std::string> filenames;
			
			if (filenameFilter.empty())
				filenames = allFilenames;
			else
			{
				for (auto & filename : allFilenames)
					if (strcasestr(filename.c_str(), filenameFilter.c_str()) != nullptr)
						filenames.push_back(filename);
			}
			
			if (keyboard.wentDown(SDLK_RETURN) && filenames.size() == 1)
			{
				auto & filename = filenames[0];
				
				JsusFxElem * elem = new JsusFxElem(effectChain.pathLibrary);
		
				elem->filename = filename;
				
				elem->init();
			
				if (elem->isValid)
				{
					audioStream.lock();
					{
						effectChain.add(elem);
					}
					audioStream.unlock();
					
					JsusFxWindow * window = new JsusFxWindow(elem);
					
					windows.push_back(window);
				}
				else
				{
					delete elem;
					elem = nullptr;
				}
			}
			
			// filtered effect list
			
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
					JsusFxElem * elem = new JsusFxElem(effectChain.pathLibrary);
					
					elem->filename = filename;
					
					elem->init();
					
					if (elem->isValid)
					{
						audioStream.lock();
						{
							effectChain.add(elem);
						}
						audioStream.unlock();
						
						JsusFxWindow * window = new JsusFxWindow(elem);
						
						windows.push_back(window);
					}
					else
					{
						delete elem;
						elem = nullptr;
					}
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
	
#if 1
	audioIO.Stop();
	audioIO.Shutdown();
#else
	audioOutput.Stop();
	audioOutput.Shutdown();
#endif

	tinyxml2::XMLPrinter p;
	
	p.OpenElement("effectChain");
	{
		effectChain.saveXml(p);
	}
	p.CloseElement();
	
	p.OpenElement("editor");
	{
		p.OpenElement("midiKeyboard");
		{
			p.PushAttribute("octave", midiKeyboard.octave);
		}
		p.CloseElement();
		
		p.OpenElement("audioInput");
		{
			p.PushAttribute("enabled", audioStream.enableInput);
		}
		p.CloseElement();
		
		p.OpenElement("effectChain");
		{
			for (JsusFxWindow * window : windows)
			{
				p.OpenElement("effect");
				{
					int windowX;
					int windowY;
					
					window->window->getPosition(windowX, windowY);
					
					p.PushAttribute("id", window->effectElem->id);
					p.PushAttribute("window_visible", !window->window->isHidden());
					p.PushAttribute("window_x", windowX);
					p.PushAttribute("window_y", windowY);
					p.PushAttribute("window_sx", window->window->getWidth());
					p.PushAttribute("window_sy", window->window->getHeight());
				}
				p.CloseElement();
			}
		}
		p.CloseElement();
	}
	p.CloseElement();
	
	tinyxml2::XMLDocument d;
	
	if (d.Parse(p.CStr()) != tinyxml2::XML_SUCCESS)
	{
		logError("failed to parse emitted XML");
		Assert(false);
	}
	else
	{
		const char * filename = "effectChain.xml";
		
		FILE * f = fopen(filename, "wt");
		
		if (f == nullptr)
		{
			logError("failed to open file for writing");
		}
		else
		{
			fprintf(f, "%s", p.CStr());
			
			fclose(f);
			f = nullptr;
		}
	}
	
	// todo : free effects, windows etc
	
	framework.shutdown();
}

int main(int argc, char * argv[])
{
#if defined(CHIBI_RESOURCE_PATH)
	changeDirectory(CHIBI_RESOURCE_PATH);
#else
	changeDirectory(SDL_GetBasePath());
#endif

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
	
	if (!fx.compile(pathLibrary, filename,
		JsusFx::kCompileFlag_CompileGraphicsSection |
		JsusFx::kCompileFlag_CompileSerializeSection))
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
			int sx;
			int sy;
			doMidiKeyboard(midiKeyboard, mouse.x - 10 - fx.gfx_w, mouse.y - 10, s_midiBuffer.bytes + s_midiBuffer.numBytes, &s_midiBuffer.numBytes, true, false, sx, sy);
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
				
				int sx;
				int sy;
				doMidiKeyboard(midiKeyboard, mouse.x - 10 - fx.gfx_w, mouse.y - 10, nullptr, nullptr, false, true, sx, sy);
			}
			gxPopMatrix();
		}
		framework.endDraw();
	}

	Font("calibri.ttf").saveCache();
	
	framework.shutdown();

	return 0;
}
