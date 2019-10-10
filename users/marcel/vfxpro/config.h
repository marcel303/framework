#pragma once

#ifdef DEBUG
	#define ENABLE_DEBUG_TEXT 1
	#define ENABLE_WINDOWED_MODE 1
	#define ENABLE_DEBUG_MENUS 1
	#define ENABLE_DEBUG_INFOS 1
	#define ENABLE_REALTIME_EDITING 1
	#define ENABLE_DEBUG_PCMTEX 0
	#define ENABLE_DEBUG_FFTTEX 0
	#define ENABLE_LOADTIME_PROFILING 0
	#define ENABLE_UPSCALING 0
	#define ENABLE_STRESS_TEST 0
	#define ENABLE_RESOURCE_PRECACHE 0
	#define ENABLE_LEAPMOTION 0
	#define ENABLE_MIDI 0
	#define ENABLE_VIDEO 1
#else
	#define ENABLE_DEBUG_TEXT 0
	#define ENABLE_WINDOWED_MODE 0
	#define ENABLE_DEBUG_MENUS 0
	#define ENABLE_DEBUG_INFOS 0
	#define ENABLE_REALTIME_EDITING 0
	#define ENABLE_DEBUG_PCMTEX 0
	#define ENABLE_DEBUG_FFTTEX 0
	#define ENABLE_LOADTIME_PROFILING 0
	#define ENABLE_UPSCALING 0
	#define ENABLE_STRESS_TEST 0
	#define ENABLE_RESOURCE_PRECACHE 1
	#define ENABLE_LEAPMOTION 0
	#define ENABLE_MIDI 0
	#define ENABLE_VIDEO 1
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

	struct Display
	{
		Display();
		
		int sx;
		int sy;
		bool fullscreen;
		bool showTestImage;
		bool showScaleOverlay;
		float gamma;
		bool mirror;
	};

	struct Debug
	{
		bool showMessages;
	};

	Midi midi;
	AudioIn audioIn;
	Display display;
	Debug debug;

	Config();

	void reset();
	bool load(const char * filename);
	
	// helpers
	
	bool midiIsMapped(int id) const;
};
