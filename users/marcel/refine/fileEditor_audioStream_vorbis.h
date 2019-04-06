#pragma once

#include "audiooutput/AudioOutput_PortAudio.h"
#include "audiostream/AudioStreamVorbis.h"
#include "fileEditor.h"

struct FileEditor_AudioStream_Vorbis : FileEditor
{
	AudioOutput_PortAudio audioOutput;
	AudioStream_Vorbis * audioStream = nullptr;
	
	FileEditor_AudioStream_Vorbis(const char * path);
	virtual ~FileEditor_AudioStream_Vorbis();
	
	virtual void tick(const int sx, const int sy, const float dt, const bool hasFocus, bool & inputIsCaptured) override;
};
