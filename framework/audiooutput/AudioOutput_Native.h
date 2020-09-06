#pragma once

#if defined(IPHONEOS) || defined(MACOS)
	#include "AudioOutput_CoreAudio.h"
	typedef AudioOutput_CoreAudio AudioOutput_Native;
#elif defined(ANDROID)
	#include "AudioOutput_OpenSL.h"
	typedef AudioOutput_OpenSL AudioOutput_Native;
#else
	#include "AudioOutput_PortAudio.h"
	typedef AudioOutput_PortAudio AudioOutput_Native;
#endif
