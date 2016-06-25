#pragma once

#ifdef DEBUG
	#define ENABLE_DEBUG_TEXT 0
	#define ENABLE_WINDOWED_MODE 0
	#define ENABLE_DEBUG_MENUS 1
	#define ENABLE_DEBUG_INFOS 0
	#define ENABLE_REALTIME_EDITING 1
#else
	#define ENABLE_DEBUG_TEXT 0
	#define ENABLE_WINDOWED_MODE 0
	#define ENABLE_DEBUG_MENUS 0
	#define ENABLE_DEBUG_INFOS 0
	#define ENABLE_REALTIME_EDITING 1
#endif

struct Config
{
	static const int kMaxMidiMappings = 256;

	struct Midi
	{
		Midi();

		bool enabled;
		int deviceIndex;
		int mapping[kMaxMidiMappings];
	};

	struct AudioIn
	{
		AudioIn();

		bool enabled;
		int deviceIndex;
		int numChannels;
		int sampleRate;
		int bufferLength;
		float volume;
	};

	Midi midi;
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
