#include "audiooutput/AudioOutput_PortAudio.h"
#include "audiosketch.h"
#include "framework.h"

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

		return numSamples;
	}
};

void runAudiosketch(AudiosketchBase & sketch)
{
	framework.init(800, 600);

	AudioOutput_PortAudio output;
	output.Initialize(1, 44100, 256);

	MyStream stream(sketch);
	output.Play(&stream);

	for (;;)
	{
		framework.process();

		if (framework.quitRequested)
			break;
		
	// todo : make this thread safe
	// todo : use the same piano key mapping as Ableton
		if (keyboard.wentDown(SDLK_SPACE))
			sketch.noteOn(random<float>(440, 880), 1);
		if (keyboard.wentUp(SDLK_SPACE))
			sketch.noteOff(0);
	}

	output.Stop();
	output.Shutdown();
}
