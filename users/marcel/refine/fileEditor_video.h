#pragma once

#include "fileEditor.h"
#include "video.h"

struct FileEditor_Video : FileEditor
{
	MediaPlayer mp;
	AudioOutput_PortAudio audioOutput;
	
	bool hasAudioInfo = false;
	bool audioIsStarted = false;
	
	FileEditor_Video(const char * path)
	{
		MediaPlayer::OpenParams openParams;
		openParams.enableAudioStream = true;
		openParams.enableVideoStream = true;
		openParams.filename = path;
		openParams.outputMode = MP::kOutputMode_RGBA;
		openParams.audioOutputMode = MP::kAudioOutputMode_Stereo;
		
		mp.openAsync(openParams);
	}
	
	virtual ~FileEditor_Video() override
	{
		audioOutput.Shutdown();
		
		mp.close(true);
	}
	
	virtual void tick(const int sx, const int sy, const float dt, const bool hasFocus, bool & inputIsCaptured) override
	{
		// attempt to get information about the video so we can start the audio

		int channelCount;
		int sampleRate;
		
		if (hasAudioInfo == false && mp.getAudioProperties(channelCount, sampleRate))
		{
			hasAudioInfo = true;

			if (channelCount > 0)
			{
				audioIsStarted = true;
			
				audioOutput.Initialize(channelCount, sampleRate, 256);
				audioOutput.Play(&mp);
			}
		}
		
		// update presentation time stamp for the video

		if (audioIsStarted)
			mp.presentTime = mp.audioTime;
		else if (hasAudioInfo)
			mp.presentTime += dt;
		
		// update video

		mp.tick(mp.context, true);

		int videoSx;
		int videoSy;
		double duration;
		double sampleAspectRatio;

		const bool hasVideoInfo = mp.getVideoProperties(videoSx, videoSy, duration, sampleAspectRatio);

		// check if the video ended and needs to be looped

		if (mp.presentedLastFrame(mp.context))
		{
			auto openParams = mp.context->openParams;

			mp.close(false);
			mp.presentTime = 0.0;
			mp.openAsync(openParams);

			audioOutput.Shutdown();

			hasAudioInfo = false;
			audioIsStarted = false;
		}
		
		//
		
		clearSurface(0, 0, 0, 0);
		
		setColor(colorWhite);
		drawUiRectCheckered(0, 0, sx, sy, 8);
		
		auto texture = mp.getTexture();
		
		if (texture != 0 && hasVideoInfo)
		{
			videoSx *= sampleAspectRatio;
			
			pushBlend(BLEND_OPAQUE);
			{
				const float scaleX = sx / float(videoSx);
				const float scaleY = sy / float(videoSy);
				const float scale = fminf(1.f, fminf(scaleX, scaleY));
				
				gxPushMatrix();
				{
					gxTranslatef((sx - videoSx * scale) / 2, (sy - videoSy * scale) / 2, 0);
					gxScalef(scale, scale, 1.f);
					
					gxSetTexture(texture);
					setColor(colorWhite);
					drawRect(0, 0, videoSx, videoSy);
					gxSetTexture(0);
				}
				gxPopMatrix();
			}
			popBlend();
		}
	}
};
