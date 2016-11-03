#pragma once

#include "audiostream/AudioStream.h"

class AudioStreamEx : public AudioStream
{
public:
	virtual int GetSampleRate() = 0;
};

extern bool g_wantsAudioPlayback;

void openAudio(AudioStreamEx * audioStream);
void closeAudio();
