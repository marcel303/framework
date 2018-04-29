#include "framework.h"

#include "jsusfx.h"
#include "jsusfx_file.h"
#include "jsusfx_gfx.h"

#include "Path.h"
#include "StringEx.h"

#include <fstream>

#include "WDL/wdlstring.h"

#include "../jsusfx-framework/gfx-framework.cpp"
/*

GFX status:
- Most of Liteon scripts appear to work.
- JSFX-Kawa (https://github.com/kawaCat/JSFX-kawa) mostly works. Some text and blitting not working yet.
- ATK for Reaper loads ok, but doesn't output anything to the screen at all.

*/

/*

import filename -- REAPER v4.25+

You can specify a filename to import (this filename will be searched within the JS effect directory). Importing files via this directive will have any functions defined in their @init sections available to the local effect. Additionally, if the imported file implements other sections (such as @sample, etc), and the importing file does not implement those sections, the imported version of those sections will be used.

Note that files that are designed for import only (such as function libraries) should ideally be named xyz.jsfx-inc, as these will be ignored in the user FX list in REAPER.

---

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

gfx_texth
	avergage height of a text line

gfx_mode
	0x1 = additive
	0x2 = disable source alpha for gfx_blit (make it opaque)
	0x4 = disable filtering for gfx_blit (point sampling)

*/

#define SAMPLE_RATE 44100
#define BUFFER_SIZE 64

#define DATA_ROOT "/Users/thecat/Library/Application Support/REAPER/Data/"

const int GFX_SX = 1000;
const int GFX_SY = 720;

class JsusFxTest : public JsusFx {
public:
	JsusFxTest(JsusFxPathLibrary &pathLibrary)
		: JsusFx(pathLibrary) {
	}
	
    virtual void displayMsg(const char *fmt, ...) override {
        char output[4096];
        va_list argptr;
        va_start(argptr, fmt);
        vsnprintf(output, 4095, fmt, argptr);
        va_end(argptr);

        logDebug(output);
    }

    virtual void displayError(const char *fmt, ...) override {
        char output[4096];
        va_list argptr;
        va_start(argptr, fmt);
        vsnprintf(output, 4095, fmt, argptr);
        va_end(argptr);

        logError(output);
    }
};

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

struct AudioStream_JsusFx : AudioStream
{
	AudioStream * source = nullptr;
	
	JsusFx * fx = nullptr;
	
	SDL_mutex * mutex = nullptr;
	
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
			buffer[i].channel[0] = outputL[i] * float(1 << 15);
			buffer[i].channel[1] = outputR[i] * float(1 << 15);
		}
		
		return numSamples;
	}
};

//

static JsusFxTest * s_fx = nullptr;

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

int main(int argc, char * argv[])
{
	framework.filedrop = true;
	framework.actionHandler = handleAction;
	
	if (!framework.init(0, nullptr, GFX_SX, GFX_SY))
		return -1;
	
	JsusFx::init();
	
	JsusFxPathLibrary_Basic pathLibrary(DATA_ROOT);
	pathLibrary.addSearchPath("lib");
	
	JsusFxTest fx(pathLibrary);
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
	const char * filename = "/Users/thecat/Downloads/JSFX-kawa-master/kawa_XY_Flanger.jsfx";
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
			
			gfx.setup(fx.gfx_w, fx.gfx_h);
			
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
