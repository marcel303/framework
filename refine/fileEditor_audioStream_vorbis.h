#pragma once

#include "audiooutput/AudioOutput_Native.h"
#include "fileEditor.h"

class AudioStream_Vorbis;

struct FileEditor_AudioStream_Vorbis : FileEditor
{
	AudioOutput_Native audioOutput;
	AudioStream_Vorbis * audioStream = nullptr;
	
	FileEditor_AudioStream_Vorbis(const char * path);
	virtual ~FileEditor_AudioStream_Vorbis();
	
	virtual void tick(const int sx, const int sy, const float dt, const bool hasFocus, bool & inputIsCaptured) override;
};
