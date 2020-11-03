#include "audiostream/AudioStreamWave.h"
#include "fileEditor_audioStream_wave.h"

FileEditor_AudioStream_Wave::FileEditor_AudioStream_Wave(const char * path)
{
	audioStream = new AudioStreamWave();
	audioStream->Open(path, true);
	
	audioOutput.Initialize(2, audioStream->SampleRate_get(), 256);
	audioOutput.Play(audioStream);
}

FileEditor_AudioStream_Wave::~FileEditor_AudioStream_Wave()
{
	audioOutput.Shutdown();
	
	delete audioStream;
	audioStream = nullptr;
}

void FileEditor_AudioStream_Wave::tick(const int sx, const int sy, const float dt, const bool hasFocus, bool & inputIsCaptured)
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
