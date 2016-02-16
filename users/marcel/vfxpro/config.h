#pragma once

#include <string.h>

struct Config
{
	static const int kMaxMidiMappings = 256;

	struct AudioIn
	{
		AudioIn()
		{
			memset(this, 0, sizeof(*this));
		}

		bool enabled;
		int numChannels;
		int sampleRate;
		int bufferLength;
		float volume;
	};

	bool midiEnabled;
	int midiMapping[kMaxMidiMappings];

	AudioIn audioIn;

	Config();

	void reset();
	bool load(const char * filename);

	// helpers

	bool midiIsMapped(int id) const;
	bool midiIsDown(int id) const;
	bool midiWentDown(int id) const;
	bool midiWentUp(int id) const;
	float midiGetValue(int id, float _default) const;
};
