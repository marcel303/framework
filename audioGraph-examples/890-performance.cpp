#include "890-performance.h"
#include "framework.h"
#include "objects/audioSourceVorbis.h"
#include "objects/binauralizer.h"
#include "objects/binaural_cipic.h"
#include "objects/paobject.h"
#include "Quat.h"
#include "soundmix.h"
#include "soundVolume.h"
#include "StringEx.h"
#include "vfxGraph.h"
#include "vfxNodes/vfxNodeDisplay.h"
#include "video.h"
#include <atomic>

extern const int GFX_SX;
extern const int GFX_SY;

#if 1
const int GFX_SX = 1024;
const int GFX_SY = 768;
#elif 1
const int GFX_SX = 2400;
const int GFX_SY = 1200;
#else
const int GFX_SX = 640;
const int GFX_SY = 480;
#endif

namespace Videotube
{
	void main();
}

SDL_mutex * g_audioMutex = nullptr;
binaural::Mutex * g_binauralMutex = nullptr;
binaural::HRIRSampleSet * g_sampleSet = nullptr;
AudioMixer * g_audioMixer = nullptr;

struct MyMutex : binaural::Mutex
{
	SDL_mutex * mutex;
	
	MyMutex(SDL_mutex * _mutex)
		: mutex(_mutex)
	{
	}
	
	virtual void lock() override
	{
		const int r = SDL_LockMutex(mutex);
		Assert(r == 0);
	}
	
	virtual void unlock() override
	{
		const int r = SDL_UnlockMutex(mutex);
		Assert(r == 0);
	}
};

int main(int argc, char * argv[])
{
    framework.enableDepthBuffer = true;
    framework.enableRealTimeEditing = true;
    
	if (!framework.init(0, nullptr, GFX_SX, GFX_SY))
		return -1;
	
	SDL_mutex * audioMutex = SDL_CreateMutex();
	g_audioMutex = audioMutex;
	
	MyMutex binauralMutex(audioMutex);
	g_binauralMutex = &binauralMutex;
	
	binaural::HRIRSampleSet sampleSet;
	binaural::loadHRIRSampleSet_Cipic("subject147", sampleSet);
	sampleSet.finalize();
	g_sampleSet = &sampleSet;
	
	AudioMixer * audioMixer = new AudioMixer();
	audioMixer->init(audioMutex);
	g_audioMixer = audioMixer;
	
    PortAudioObject pa;
	pa.init(SAMPLE_RATE, 2, 0, AUDIO_UPDATE_SIZE, audioMixer);
	
    Videotube::main();
	
	pa.shut();
	
	g_audioMixer->shut();
	delete g_audioMixer;
	g_audioMixer = nullptr;
	
	g_binauralMutex = nullptr;
	SDL_DestroyMutex(audioMutex);
	audioMutex = nullptr;
	
	Font("calibri.ttf").saveCache();
	
	framework.shutdown();
	
	return 0;
}
