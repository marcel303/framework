#pragma once

#include "audiooutput/AudioOutput_PortAudio.h"
#include "fileEditor.h"
#include "video.h"

struct FileEditor_Video : FileEditor
{
	MediaPlayer mp;
	AudioOutput_PortAudio audioOutput;
	
	bool hasAudioInfo = false;
	bool audioIsStarted = false;
	
	FileEditor_Video(const char * path);
	virtual ~FileEditor_Video() override;
	
	virtual void tick(const int sx, const int sy, const float dt, const bool hasFocus, bool & inputIsCaptured) override;
};
