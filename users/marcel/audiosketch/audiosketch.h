#pragma once

#include <math.h>
#include <stdint.h>

struct AudiosketchBase
{
	int sampleRate = 0;
	
	virtual void noteOn(float frequency, float velocity) { }
	virtual void noteOff(float velocity) { }
	
	virtual void block() { }

	virtual float sample() { return 0; }
};

void runAudiosketch(AudiosketchBase & sketch);

#define AUDIOSKETCH(name) struct name : AudiosketchBase
	
#define AUDIOSKETCH_MAIN(SketchType) \
	int main(int argc, char * argv[]) \
	{ \
		SketchType sketch; \
		runAudiosketch(sketch); \
		return 0; \
	}
