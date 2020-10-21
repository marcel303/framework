#pragma once

#include "audiooutput/AudioOutput_Native.h"
#include "fileEditor.h"
#include "video.h"

struct FileEditor_Video : FileEditor
{
	enum SizeMode
	{
		kSizeMode_Contain,
		kSizeMode_Fill,
		kSizeMode_DontScale,
		kSizeMode_COUNT
	};
	
	MediaPlayer mp;
	AudioOutput_Native audioOutput;
	
	bool hasAudioInfo = false;
	bool audioIsStarted = false;
	
	float progressBarTimer = 0.f;
	
	SizeMode sizeMode = kSizeMode_Contain;
	
	FileEditor_Video(const char * path);
	virtual ~FileEditor_Video() override;
	
	virtual bool reflect(TypeDB & typeDB, StructuredType & type) override;
	
	virtual void tick(const int sx, const int sy, const float dt, const bool hasFocus, bool & inputIsCaptured) override;
	
	virtual void doButtonBar() override;
};
