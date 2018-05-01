#include "audioMixer.h"
#include "objects/binauralizer.h"

extern SDL_mutex * g_audioMutex;
extern binaural::Mutex * g_binauralMutex;
extern binaural::HRIRSampleSet * g_sampleSet;
extern AudioMixer * g_audioMixer;
