#pragma once

#if defined(IPHONEOS)
	#include "AudioOutputHD_CoreAudio.h"
	typedef AudioOutputHD_CoreAudio AudioOutputHD_Native;
#elif AUDIOOUTPUT_HD_USE_OPENSL
	#include "AudioOutputHD_OpenSL.h"
	typedef AudioOutputHD_OpenSL AudioOutputHD_Native;
#else
	#include "AudioOutputHD_PortAudio.h"
	typedef AudioOutputHD_PortAudio AudioOutputHD_Native;
#endif
