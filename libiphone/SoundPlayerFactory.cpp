#include "SoundPlayerFactory.h"

#ifdef IPHONEOS
	#include "SoundPlayerV3.h"
#endif
#if defined(WIN32) || defined(LINUX) || defined(MACOS) || defined(IPAD) || defined(BBOS)
	#include "SoundPlayer_OpenAL.h"
#endif
#ifdef PSP
	#include "SoundPlayer_Psp.h"
#endif

ISoundPlayer* SoundPlayerFactory::Create()
{
#if defined(IPAD)
	return new SoundPlayer_OpenAL();
#elif defined(IPHONEOS)
	return new SoundPlayerV3();
#elif defined(WIN32)
	return new SoundPlayer_OpenAL();
#elif defined(LINUX)
	return new SoundPlayer_OpenAL();
#elif defined(MACOS)
	return new SoundPlayer_OpenAL();
#elif defined(BBOS)
	return new SoundPlayer_OpenAL();
#elif defined(PSP)
	return new SoundPlayer_Psp();
#else
#error "unknown system"
#endif
}
