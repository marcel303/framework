#pragma once

#include "audiooutput/AudioOutput_PortAudio.h"
#include "audiostream/AudioStreamVorbis.h"

struct FileEditor_AudioStream_Vorbis : FileEditor
{
	AudioOutput_PortAudio audioOutput;
	AudioStream_Vorbis * audioStream = nullptr;
	
	FileEditor_AudioStream_Vorbis(const char * path)
	{
		audioStream = new AudioStream_Vorbis();
		audioStream->Open(path, true);
		
		audioOutput.Initialize(2, audioStream->SampleRate_get(), 256);
		audioOutput.Play(audioStream);
	}
	
	virtual ~FileEditor_AudioStream_Vorbis()
	{
		audioOutput.Shutdown();
		
		delete audioStream;
		audioStream = nullptr;
	}
	
	virtual void tick(const int sx, const int sy, const float dt, const bool hasFocus, bool & inputIsCaptured) override
	{
		audioOutput.Update();
		
		if (hasFocus == false)
			return;
		
		clearSurface(0, 0, 0, 0);
		
		if (audioStream->Duration_get() > 0)
		{
			gxPushMatrix();
			gxTranslatef(0, sy/2, 0);
			
			hqBegin(HQ_FILLED_ROUNDED_RECTS);
			setColor(100, 100, 200);
			hqFillRoundedRect(4, -10, int64_t(sx - 4) * audioStream->Position_get() / audioStream->Duration_get(), +10, 10);
			hqEnd();
			
			hqBegin(HQ_STROKED_ROUNDED_RECTS);
			setColor(140, 140, 200);
			hqStrokeRoundedRect(4, -10, sx - 4, +10, 10, 2.f);
			hqEnd();
			
			gxPopMatrix();
		}
	}
};
