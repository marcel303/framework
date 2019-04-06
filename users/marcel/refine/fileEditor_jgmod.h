#pragma once

#include "fileEditor.h"
#include "framework-allegro2.h"
#include "jgmod.h"
#include "jgvis.h"

#include "allegro2-timerApi.h"
#include "allegro2-voiceApi.h"

#include "audiooutput/AudioOutput_PortAudio.h"

struct FileEditor_Jgmod : FileEditor
{
	AllegroTimerApi timerApi;
	AllegroVoiceApi voiceApi;
	
	JGMOD * jgmod = nullptr;
	
	JGMOD_PLAYER player;
	
	JGVIS vis;
	
	AudioOutput_PortAudio audioOutput;
	AudioStream_AllegroVoiceMixer voiceMixer;
	
	FileEditor_Jgmod(const char * path);
	virtual ~FileEditor_Jgmod() override;
	
	virtual bool wantsTick(const bool hasFocus, const bool inputIsCaptured) override;
	
	virtual void doButtonBar() override;
	
	virtual void tick(const int sx, const int sy, const float dt, const bool hasFocus, bool & inputIsCaptured) override;
};
