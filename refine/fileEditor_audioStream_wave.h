#pragma once

#include "audiooutput/AudioOutput_Native.h"
#include "fileEditor.h"

class AudioStreamWave;

struct FileEditor_AudioStream_Wave : FileEditor
{
	AudioOutput_Native audioOutput;
	AudioStreamWave * audioStream = nullptr;
	
	FileEditor_AudioStream_Wave(const char * path);
	virtual ~FileEditor_AudioStream_Wave();
	
	virtual void tick(const int sx, const int sy, const float dt, const bool hasFocus, bool & inputIsCaptured) override;
};
