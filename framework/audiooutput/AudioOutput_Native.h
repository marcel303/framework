#pragma once

#if defined(IPHONEOS)
	// todo : include macOS in. requires to fix initialization on osx
	#include "AudioOutput_CoreAudio.h"
	typedef AudioOutput_CoreAudio AudioOutput_Native;
#elif defined(ANDROID)
	#include "AudioOutput_OpenSL.h"
	typedef AudioOutput_OpenSL AudioOutput_Native;
#else
	#include "AudioOutput_PortAudio.h"
	typedef AudioOutput_PortAudio AudioOutput_Native;
#endif
