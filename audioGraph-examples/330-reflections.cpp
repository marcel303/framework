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

#include "audioGraph.h"
#include "audioGraphManager.h"
#include "audioUpdateHandler.h"
#include "framework.h"
#include "soundmix.h"
#include "vfxNodes/delayLine.h"
#include <cmath>
#include <map>

#if !defined(DEBUG)
	#define FULLSCREEN 1
#else
	#define FULLSCREEN 0 // do not alter
#endif

#if FULLSCREEN
	const int GFX_SX = 800;
	const int GFX_SY = 600;
#else
	const int GFX_SX = 1400;
	const int GFX_SY = 400;
#endif

#define CHANNEL_COUNT 16

#if 0
	#define NUM_PARTICLES 1
	#define MAX_SPACE_POINTS 10
#else
	#define NUM_PARTICLES 1
	#define MAX_SPACE_POINTS 200
#endif

#define ENABLE_MIDI 0

#define ENABLE_GAMEPAD 1

#if ENABLE_MIDI
	#include "objects/mididecoder.h"
	#include "rtmidi/RtMidi.h"
#endif

//

static double s_morph1 = 0.0;
static double s_morph2 = 1.0;
static double s_speed = 0.0;

//

static int s_tickCount = 0;

struct ALIGN32 AudioBuffer
{
	float samples[AUDIO_UPDATE_SIZE];
};

struct SourceParticle
{
	Vec3 position;
	
	AudioBuffer audioBuffer;
};

struct Source
{
	int tickCount = -1;
	int downMixTick = -1;
	
	Vec3 position;
	
	AudioSource * source = nullptr;
	
	AudioBuffer input;
	
	SourceParticle particles[NUM_PARTICLES];
	
	AudioBuffer monoOutput;
	
	Vec3 monoPosition;
	
	double phase1 = 0.0;
	double phase2 = 0.0;
	double phaseStep1 = random(3.0, 50.0) / SAMPLE_RATE * M_PI * 2.0;
	double phaseStep2 = random(5.0, 250.0) / SAMPLE_RATE * M_PI * 2.0;
	
	void tick()
	{
		if (tickCount == s_tickCount)
			return;
		
		if (mouse.wentDown(BUTTON_LEFT))
		{
			phaseStep1 = random(3.0, 200.0) / SAMPLE_RATE * M_PI * 2.0;
			phaseStep2 = random(10.0, 250.0) / SAMPLE_RATE * M_PI * 2.0;
		}
		
		tickCount = s_tickCount;
		
		if (source != nullptr)
		{
			source->generate(input.samples, AUDIO_UPDATE_SIZE);
		}
		else
		{
			for (int i = 0; i < AUDIO_UPDATE_SIZE; ++i)
			{
				input.samples[i] = sinf(phase1 * 2.0 * M_PI) * sinf(phase2 * 2.0 * M_PI) * .01f;
				
				phase1 += phaseStep1;
				phase2 += phaseStep2;
				
				if (phase1 >= 1.0)
					phase1 -= 1.0;
				if (phase2 >= 1.0)
					phase2 -= 1.0;
			}
		}
			
		for (int i = 0; i < NUM_PARTICLES; ++i)
		{
			auto & p = particles[i];
			
			const float t = tickCount * AUDIO_UPDATE_SIZE / float(SAMPLE_RATE) * (i + 1) / float(NUM_PARTICLES) * .5f;
			
			p.position = Vec3(std::sin(t / 2.156) * 16.f, std::cos(t / 3.843f) * 14.f, 0.f);
			
			p.audioBuffer = input;
		}
	}
	
	void downMix()
	{
		Assert(tickCount == s_tickCount);
		
		if (downMixTick == s_tickCount)
			return;
		
		downMixTick = s_tickCount;
		
		//
		
		{
			Vec3 sum;
			
			for (auto & p : particles)
			{
				sum += p.position;
			}
			
			monoPosition = sum / NUM_PARTICLES;
		}
		
		//
		
		for (int i = 0; i < AUDIO_UPDATE_SIZE; ++i)
		{
			float sum = 0.f;
			
			for (auto & p : particles)
			{
				sum += p.audioBuffer.samples[i];
			}
			
			monoOutput.samples[i] = sum;
		}
	}
};

#define USE_FIXEDPOINT_SAMPLING 0
#define DO_INTERPOLATION 1

struct SpacePoint
{
	Vec3 position;
	Vec3 direction;
	
	AudioBuffer output;
	
	void beginMixing()
	{
		memset(&output, 0, sizeof(output));
	}
	
	void processReflection(const AudioBuffer & sourceBuffer, Vec3Arg sourcePosition, const DelayLine & delayLine, float & previousReadOffset, float & previousGain)
	{
		const Vec3 delta = sourcePosition - position;
		
		const Vec3 direction2 = delta.CalcNormalized();
		const float reflectionGain = std::max(0.01f, direction2 * direction);
		
		const float distance = delta.CalcSize() * 10.f;
		
		const float readOffset1 = previousReadOffset + AUDIO_UPDATE_SIZE;
		const float readOffset2 = distance         / 340.f * SAMPLE_RATE;
		
		//printf("%d (%d) -> %d (%d)\n", readOffset1, readOffset2, readOffset1 - AUDIO_UPDATE_SIZE, readOffset1 - readOffset2);
		
		const float gain1 = previousGain;
		const float gain2 = 10.f / (MAX_SPACE_POINTS * NUM_PARTICLES) * reflectionGain;
		//const float gain2 = 10.f / ((distance) + .1f) / MAX_SPACE_POINTS;
		
		const float invAudioUpdateSize = 1.f / (AUDIO_UPDATE_SIZE - 1);
		
	#if USE_FIXEDPOINT_SAMPLING
		const int kNumBits = 10;
		const int kAudioBits = 8;
		
		const float toInterp = 1.f / (1 << kNumBits);
		
		const int64_t readOffset1i = readOffset1 * int64_t(1 << kNumBits);
		const int64_t readOffset2i = readOffset2 * int64_t(1 << kNumBits);
		
		int64_t readOffseti = readOffset1i * (AUDIO_UPDATE_SIZE - 1);
		int64_t readOffsetInci = readOffset2i - readOffset1i;
	#endif
	
		for (int i = 0; i < AUDIO_UPDATE_SIZE; ++i)
		{
			float sample;
			
		#if USE_FIXEDPOINT_SAMPLING
			const int readOffset_fine = readOffseti >> kAudioBits;
			const int readOffset = readOffset_fine >> kNumBits;
			
		#if DO_INTERPOLATION
			int index1 = delayLine.nextWriteIndex - 1 - readOffset;
			int index2 = index1 - 1;
			
			if (index2 < 0)
			{
				index2 += delayLine.numSamples;
				
				if (index1 < 0)
					index1 += delayLine.numSamples;
			}
			
			const float readOffset_interpAmount = (readOffset_fine - (readOffset << kNumBits)) * toInterp;
			
			const float sample1 = delayLine.samples[index1];
			const float sample2 = delayLine.samples[index2];
			
			const float t1 = 1.f - readOffset_interpAmount;
			const float t2 = readOffset_interpAmount;
			
			sample = sample1 * t1 + sample2 * t2;
		#else
			int index = delayLine.nextWriteIndex - 1 - readOffset;
			
			if (index < 0)
				index += delayLine.numSamples;
			
			sample = delayLine.samples[index];
		#endif
			
			//
			
			readOffseti += readOffsetInci;
		#else
			const float readOffset = (readOffset2 * i + readOffset1 * (AUDIO_UPDATE_SIZE - i - 1)) * invAudioUpdateSize;
			const int readOffseti = int(readOffset);
			
			const float readOffset_interpAmount = readOffset - readOffseti;
			
			int index1 = delayLine.nextWriteIndex - 1 - readOffseti;
			int index2 = index1 - 1;
			
			if (index2 < 0)
			{
				index2 += delayLine.numSamples;
				
				if (index1 < 0)
					index1 += delayLine.numSamples;
			}
			
			const float sample1 = delayLine.samples[index1];
			
		#if DO_INTERPOLATION
			const float sample2 = delayLine.samples[index2];
			
			const float t1 = 1.f - readOffset_interpAmount;
			const float t2 = readOffset_interpAmount;
			
			sample = sample1 * t1 + sample2 * t2;
		#else
			sample = sample1;
		#endif
		#endif
			
			const float gain = (gain2 * i + gain1 * (AUDIO_UPDATE_SIZE - i - 1)) * invAudioUpdateSize;
			
			output.samples[i] += sample * gain;
		}
		
		previousReadOffset = readOffset2;
		previousGain = gain2;
	}
};

struct Space
{
	enum InputType
	{
		kInputType_Mono,
		kInputType_Particles
	};
	
	int tickCount = -1;
	
	//InputType inputType = kInputType_Mono;
	InputType inputType = kInputType_Particles;
	
	Source * source = nullptr;
	
	SpacePoint points[MAX_SPACE_POINTS];
	
	struct LinkId
	{
		Source * source = nullptr;
		int particleIndex = -1;
		int pointIndex = -1;
		
		bool operator<(const LinkId & other) const
		{
			if (particleIndex != other.particleIndex)
				return particleIndex < other.particleIndex;
			if (pointIndex != other.pointIndex)
				return pointIndex < other.pointIndex;
			return false;
		}
	};
	
	struct LinkData
	{
		float previousReadOffset = 0.f;
		float previousGain = 0.f;
		
		DelayLine delayLine;
		
		LinkData()
		{
			delayLine.setLength(SAMPLE_RATE * 3);
		}
	};
	
	std::map<LinkId, LinkData*> links;
	
	double t = 0.0;
	
	static Vec3 evalCircle(const double t, const float radius)
	{
		Vec3 p;
		
		p[0] = std::cos(t * 2.0 * M_PI);
		p[1] = std::sin(t * 2.0 * M_PI);
		
		return p * radius;
	}
	
	static Vec3 evalQuad(const double t, const float radius)
	{
		const double u = fmod(fabs(t), 1.0) * 4.0;
		const int s = int(u);
		const double f = u - s;
		
		Vec3 p;
		
		if (s == 0)
		{
			p[0] = -1.f + f * 2.f;
			p[1] = -1.f;
		}
		else if (s == 1)
		{
			p[0] = +1.f;
			p[1] = -1.f + f * 2.f;
		}
		else if (s == 2)
		{
			p[0] = +1.f - f * 2.f;
			p[1] = +1.f;
		}
		else if (s == 3)
		{
			p[0] = -1.f;
			p[1] = +1.f - f * 2.f;
		}
		
		return p * radius;
	}
	
	static Vec3 evalSnake(const double t)
	{
		Vec3 p;
		
		p[0] = std::cos(t * 2.0 * M_PI / 2.345) * 16.f;
		p[1] = std::sin(t * 2.0 * M_PI / 1.234) * 6.f;
		
		return p;
	}
	
	static Vec3 evalParticlePosition(const float i, const double t)
	{
	#if !ENABLE_MIDI
		if (keyboard.isDown(SDLK_a))
			s_morph1 = clamp(s_morph1 + mouse.dy / 100000.0, 0.0, 1.0);
		if (keyboard.isDown(SDLK_s))
			s_morph2 = clamp(s_morph2 + mouse.dy / 100000.0, 0.0, 1.0);
	#endif
	
		//
		
		const double pt = t * (i + .5f) / 6.f;
		
		auto p1 = evalQuad(pt / 1.123, 12.f);
		auto p2 = evalCircle(pt, 12.f);
		auto p3 = evalSnake(pt / 1.234);
		
		Vec3 p = p1;
		
		p = lerp(p, p2, s_morph1);
		p = lerp(p, p3, s_morph2);
		
		return p;
	}
	
	void tickParticles()
	{
	#if !ENABLE_MIDI && !ENABLE_GAMEPAD
		s_speed = lerp(-1.f / 500.f, 1.f / 500.f, mouse.x / float(GFX_SX));
	#endif
	
		t += s_speed;
		
		for (int i = 0; i < MAX_SPACE_POINTS; ++i)
		{
			auto & p = points[i];
			
			p.position = evalParticlePosition(i, t);
			
			Vec3 d = evalParticlePosition(i + .01f, t) - evalParticlePosition(i - .01f, t);
			p.direction[0] = -d[1];
			p.direction[1] = +d[0];
			p.direction.Normalize();
		}
	}
	
	void tick()
	{
		if (tickCount == s_tickCount)
			return;
		
		tickCount = s_tickCount;
		
		tickParticles();
		
		for (int i = 0; i < MAX_SPACE_POINTS; ++i)
		{
			points[i].beginMixing();
		}
		
		if (source != nullptr)
		{
			source->tick();
			
			if (inputType == kInputType_Mono)
			{
				// process audio for mono down mix of all of the particle outputs
				
				source->downMix();
				
				AudioBuffer & sourceBuffer = source->monoOutput;
				Vec3 & sourcePosition = source->monoPosition;
				
				processReflection(sourceBuffer, sourcePosition, -1);
			}
			else if (inputType == kInputType_Particles)
			{
				// process audio for each particle
				
				for (int i = 0; i < NUM_PARTICLES; ++i)
				{
					AudioBuffer & audioBuffer = source->particles[i].audioBuffer;
					
					processReflection(audioBuffer, source->particles[i].position, i);
				}
			}
		}
	}
	
	void processReflection(const AudioBuffer & sourceBuffer, Vec3Arg sourcePosition, const int particleIndex)
	{
		// note : there is a modulating delay line between each space point and source point. this is what creates the 'noise effect' when points are moving quickly and the phasing when the space is configured in particular ways
		
		for (int i = 0; i < MAX_SPACE_POINTS; ++i)
		{
			auto & point = points[i];
			
			LinkId linkId;
			linkId.source = source;
			linkId.particleIndex = particleIndex;
			linkId.pointIndex = i;
			
			auto & link = links[linkId];
			
			if (link == nullptr)
				link = new LinkData();
			
			auto & delayLine = link->delayLine;
			
			for (int i = 0; i < AUDIO_UPDATE_SIZE; ++i)
				delayLine.push(sourceBuffer.samples[i]);
			
			point.processReflection(sourceBuffer, sourcePosition, delayLine, link->previousReadOffset, link->previousGain);
		}
	}
};

#if ENABLE_MIDI || ENABLE_GAMEPAD

struct Controller
{
	static const int kNumControlValues = 3;
	
	float controlValues[kNumControlValues];
	
	Controller()
	{
		memset(controlValues, 0, sizeof(controlValues));
	}
};

#endif

#if ENABLE_MIDI

struct MidiController : Controller
{
	RtMidiIn * midiIn = nullptr;
	
	~MidiController()
	{
		Assert(midiIn == nullptr);
		shut();
	}
	
	void init(const int port)
	{
		midiIn = new RtMidiIn(RtMidi::UNSPECIFIED, "Midi Controller", 1024);
		
		for (int i = 0; i < midiIn->getPortCount(); ++i)
		{
			auto name = midiIn->getPortName();
			
			logDebug("available MIDI port: %d: %s", i, name.c_str());
		}
		
		if (port < midiIn->getPortCount())
		{
			midiIn->openPort(port);
		}
	}
	
	void shut()
	{
		delete midiIn;
		midiIn = nullptr;
	}
	
	void tick()
	{
		// poll MIDI messages
		
		std::vector<uint8_t> messageBytes;
		
		for (;;)
		{
			midiIn->getMessage(&messageBytes);
			
			MidiDecoder::Message message;
			
			if (messageBytes.empty())
				break;
			
			if (message.decode(&messageBytes[0], messageBytes.size()))
			{
				if (message.type == MidiDecoder::kMessageType_ControllerChange)
				{
					logDebug("controller change. channel=%d, value=%d", message.controllerChange.note, message.controllerChange.value);
					
					// map the KORG nanoKONTROL2 midi controller's buttons and sliders
					
					int index = -1;
					
					if (message.controllerChange.note == 0) // 1st slider
						index = 0;
					if (message.controllerChange.note == 1) // 2nd slider
						index = 1;
					if (message.controllerChange.note == 16) // 1st knob
						index = 2;
					
					if (index >= 0 && index < kNumControlValues)
					{
						controlValues[index] = message.controllerChange.value / 127.f;
					}
				}
			}
		}
	}
};

#endif

#if ENABLE_GAMEPAD

struct GamepadController : Controller
{
	int desiredMorph = 0;
	
	bool wasDown = false;
	
	void tick()
	{
		controlValues[2] = (gamepad[0].getAnalog(0, ANALOG_X) / 2.f + 1.f) / 2.f;
		
		const float alpha = (clamp(gamepad[0].getAnalog(0, ANALOG_Y) * 1.5f, -1.f, +1.f) + 1.f) / 2.f;// * (.5f / .6f);
		
		const bool isDown = gamepad[0].isDown(GAMEPAD_A);
		
		if (isDown && !wasDown)
			desiredMorph = (desiredMorph + 1) % 3;
		
		wasDown = isDown;
		
		const float desiredMorphValues[2] =
		{
			desiredMorph == 1 ? .7f + alpha / .5f * .3f : desiredMorph == 0 ? (alpha - .5f) * .6f : 0.f,
			desiredMorph == 2 ? .7f + alpha / .5f * .3f : desiredMorph == 0 ? (alpha - .5f) * .0f : 0.f
		};
		
		const float retain = gamepad[0].isDown(DPAD_DOWN) ? .998f : .99f;
		const float decay = 1.f - retain;
		
		for (int i = 0; i < 2; ++i)
		{
			controlValues[i] = controlValues[i] * retain + desiredMorphValues[i] * decay;
		}
	}
};

#endif

struct MyAudioSource : AudioSource
{
	AudioSourcePcm pcm;
	
	Source * source = nullptr;

	Space * space = nullptr;

#if ENABLE_MIDI
	MidiController midiController;
#endif

#if ENABLE_GAMEPAD
	GamepadController gamepadController;
#endif

	MyAudioSource()
	{
		fillPcmDataCache("testsounds", true, true, true);
		pcm.init(getPcmData("music2.ogg"), 0);
		pcm.play();
		
		source = new Source();
		source->source = &pcm;

		space = new Space();

		space->source = source;
		
	#if ENABLE_MIDI
		midiController.init(0);
	#endif
	}
	
	virtual void generate(SAMPLE_ALIGN16 float * __restrict samples, const int numSamples) override
	{
		Controller controller;
		
	#if ENABLE_MIDI
		midiController.tick();
		
		controller = midiController;
	#endif
	
	#if ENABLE_GAMEPAD
		gamepadController.tick();
		
		controller = gamepadController;
	#endif
	
	#if ENABLE_MIDI || ENABLE_GAMEPAD
		// update control values
		
		s_morph1 = controller.controlValues[0];
		s_morph2 = controller.controlValues[1];
		
		const float speed = (controller.controlValues[2] - .5f) * 2.f;
		const float speedSign = speed < 0.f ? -1.f : +1.f;
		const float speedMag = fabs(speed);
		const float speedMagCurve = powf(speedMag, 3.f);
		s_speed = speedMagCurve * speedSign / 100.f;
	#endif
	
		s_tickCount++;
		
		space->tick();
		
		audioBufferSetZero(samples, numSamples);
		
		for (int i = 0; i < MAX_SPACE_POINTS; ++i)
			audioBufferAdd(samples, space->points[i].output.samples, numSamples);
	}
};

int main(int argc, char * argv[])
{
	// todo : let source audio come from audio graph instances
	
#if FULLSCREEN
	framework.fullscreen = true;
#endif

	if (framework.init(0, 0, GFX_SX, GFX_SY))
	{
	#if ENABLE_MIDI || ENABLE_GAMEPAD
		mouse.showCursor(false);
	#endif
	
		// initialize audio related systems
		
		SDL_mutex * mutex = SDL_CreateMutex();
		Assert(mutex != nullptr);

		AudioVoiceManagerBasic voiceMgr;
		voiceMgr.init(mutex, CHANNEL_COUNT, CHANNEL_COUNT);
		voiceMgr.outputStereo = true;

		AudioGraphManager_Basic audioGraphMgr(true);
		audioGraphMgr.init(mutex, &voiceMgr);

		AudioUpdateHandler audioUpdateHandler;
		audioUpdateHandler.init(mutex, nullptr, 0);
		audioUpdateHandler.voiceMgr = &voiceMgr;
		audioUpdateHandler.audioGraphMgr = &audioGraphMgr;
		
		MyAudioSource source;
		
		AudioVoice * voice = nullptr;
		voiceMgr.allocVoice(voice, &source, "reflection", false, 0.f, 0.f, -1);

		PortAudioObject pa;
		pa.init(SAMPLE_RATE, 2, 0, AUDIO_UPDATE_SIZE, &audioUpdateHandler);
		
		while (!framework.quitRequested)
		{
			framework.process();
			
			if (keyboard.wentDown(SDLK_ESCAPE))
				framework.quitRequested = true;
			
			audioGraphMgr.tickMain();
			
			framework.beginDraw(0, 0, 0, 0);
			{
				setFont("calibri.ttf");
				
				gxPushMatrix();
				gxTranslatef(GFX_SX/2, GFX_SY/2, 0);
				gxScalef(10, 10, 1);
				
				hqBegin(HQ_LINES, true);
				{
					setColor(255, 255, 255, 160);
					const float s = 1.6f;
					for (auto & p : source.space->points)
						hqLine(p.position[0], p.position[1], 4.f, p.position[0] + p.direction[0] * s, p.position[1] + p.direction[1] * s, 1.f);
				}
				hqEnd();
				
				hqBegin(HQ_STROKED_CIRCLES, true);
				{
					setColor(colorWhite);
					for (auto & p : source.source->particles)
						hqStrokeCircle(p.position[0], p.position[1], 10.f, 2.f);
					
					setColor(colorGreen);
					for (auto & p : source.space->points)
						hqStrokeCircle(p.position[0], p.position[1], 7.f, 1.7f);
				}
				hqEnd();
				
				gxPopMatrix();
				
			#if defined(DEBUG)
				drawText(5, 5, 12, +1, +1, "CPU usage: %d%%", (int)round(audioUpdateHandler.msecsPerSecond / 1000000.0 * 100.0));
			#endif
			}
			framework.endDraw();
		}
		
		// shut down audio related systems

		pa.shut();
		
		audioUpdateHandler.shut();

		audioGraphMgr.shut();
		
		voiceMgr.shut();

		SDL_DestroyMutex(mutex);
		mutex = nullptr;
	}
	framework.shutdown();

	return 0;
}
