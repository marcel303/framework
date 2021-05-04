#pragma once

#if AUDIOOUTPUT_HD_USE_COREAUDIO
	#include "AudioOutputHD_CoreAudio.h"
	typedef AudioOutputHD_CoreAudio AudioOutputHD_Native;
#else
	#include "AudioOutputHD_PortAudio.h"
	typedef AudioOutputHD_PortAudio AudioOutputHD_Native;
#endif
