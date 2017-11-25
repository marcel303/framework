#pragma once

#define VIDEO_RECORDING_MODE 0
#define USE_AUDIO_INPUT 0

#if defined(WIN32)
	#define ENABLE_SCRIPT_EFFECT 1
#else
	#define ENABLE_SCRIPT_EFFECT 0
#endif
