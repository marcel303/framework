#include "audiooutput/AudioOutput_Native.h"
#include "audiosketch.h"
#include "framework.h"
#include <mutex>
#include <thread>

static std::mutex s_mutex;

struct MyStream : AudioStream
{
	AudiosketchBase & sketch;

	MyStream(AudiosketchBase & in_sketch)
		: AudioStream()
		, sketch(in_sketch)
	{
	}

	virtual int Provide(int numSamples, AudioSample* __restrict buffer)
	{
		s_mutex.lock();
		
		sketch.sampleRate = 44100; // todo
		
		sketch.block();

		for (int i = 0; i < numSamples; ++i)
		{
			float value = sketch.sample();

			if (value < -1.f)
				value = -1.f;
			if (value > +1.f)
				value = +1.f;

			const int value_int = value * (1 << 15);

			buffer[i].channel[0] = value_int;
			buffer[i].channel[1] = value_int;
		}

		s_mutex.unlock();
		
		return numSamples;
	}
};

void runAudiosketch(AudiosketchBase & sketch)
{
	framework.init(800, 600);

	AudioOutput_Native output;
	output.Initialize(1, 44100, 256);

	MyStream stream(sketch);
	output.Play(&stream);

	for (;;)
	{
		framework.waitForEvents = true;
		
		framework.process();

		if (framework.quitRequested)
			break;
		
		s_mutex.lock();
		{
		// todo : use the same piano key mapping as Ableton
			if (keyboard.wentDown(SDLK_SPACE))
				sketch.noteOn(random<float>(440, 880), 1);
			if (keyboard.wentUp(SDLK_SPACE))
				sketch.noteOff(0);
			
			for (int i = 1; i <= 9; ++i)
			{
				if (keyboard.wentDown(SDLK_0 + i))
					sketch.noteOn(440.f * powf(2.f, 1.f + i / 8.f), 1);
				if (keyboard.wentUp(SDLK_0 + i))
					sketch.noteOff(0);
			}
		}
		s_mutex.unlock();
	}

	output.Stop();
	output.Shutdown();
}
