#pragma once

#ifdef IPHONEOS

#include "OpenALState.h"

void AudioSession_Initialize(bool playBackgroundMusic);
void AudioSession_Shutdown();
void AudioSession_ManageOpenAL(OpenALState* openALState);
void AudioSession_Pause();
void AudioSession_Resume();

bool AudioSession_PlayBackgroundMusic();
bool AudioSession_DetectbackgroundMusic();

#endif
