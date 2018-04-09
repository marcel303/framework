#include "audio.h"
#include "framework.h"
#include "objects/paobject.h"
#include "video.h"

#include "AmbisonicDecoder.h"

#include <atomic>

const int GFX_SX = 512 * 3/2;
const int GFX_SY = 512;

#define AUDIO_BUFFER_SIZE 32

static SDL_mutex * s_mutex = nullptr;

struct MyAudioHandler : PortAudioHandler
{
	SoundData * soundData = nullptr;
	
	int samplePosition = 0;
	
	CAmbisonicDecoder decoder;
	CBFormat bformat;
	
	double azimuth = 0.0;
	
	double azimuthSpeed = 0.0;
	
	void init(const char * filename)
	{
		soundData = loadSound("sound.wav");
		
		decoder.Configure(1, true, kAmblib_CustomSpeakerSetUp, 2);
		
		bformat.Configure(1, true, AUDIO_BUFFER_SIZE);
		bformat.Refresh();
		
		azimuthSpeed = 0.0;
	}
	
	virtual void portAudioCallback(
		const void * inputBuffer,
		const int numInputChannels,
		void * outputBuffer,
		const int framesPerBuffer) override
	{
		const double dt = AUDIO_BUFFER_SIZE / 44100.0;
		
		SDL_LockMutex(s_mutex);
		{
			if (keyboard.isDown(SDLK_LEFT))
				azimuthSpeed = -.7f;
			if (keyboard.isDown(SDLK_RIGHT))
				azimuthSpeed = +.7f;
			
			azimuth += azimuthSpeed * dt;
			
			azimuth -= dt * .03;
			
			azimuthSpeed = azimuthSpeed * pow(0.97, dt * 1000.0);
		}
		SDL_UnlockMutex(s_mutex);
		
		if (soundData != nullptr &&
			soundData->sampleCount > 0 &&
			soundData->channelSize == 2 &&
			soundData->channelCount == 4)
		{
			float samples[4][AUDIO_BUFFER_SIZE];
			
			const short * sampleData = (short*)soundData->sampleData;
			const float shortToFloat = 1.f / (1 << 15);
			
			for (int i = 0; i < AUDIO_BUFFER_SIZE; ++i)
			{
				for (int channelIndex = 0; channelIndex < 4; ++channelIndex)
				{
					const float value = sampleData[samplePosition * 4 + channelIndex] * shortToFloat;
					
					samples[channelIndex][i] = value;
				}
				
				samplePosition++;
				
				if (samplePosition == soundData->sampleCount)
					samplePosition = 0;
			}
			
			for (int channelIndex = 0; channelIndex < 4; ++channelIndex)
			{
				bformat.InsertStream(samples[channelIndex], channelIndex, AUDIO_BUFFER_SIZE);
			}
			
			float stereo[2][AUDIO_BUFFER_SIZE];
			
			float * stereoPtr[2] = { stereo[0], stereo[1] };
			
    		PolarPoint pointL;
			pointL.fAzimuth = +30.f - azimuth;
			pointL.fElevation = 10.f;
			pointL.fDistance = 0.f;
			
			PolarPoint pointR;
			pointR.fAzimuth = -30.f - azimuth;
			pointR.fElevation = 10.f;
			pointR.fDistance = 0.f;
			
			decoder.SetPosition(0, pointL);
			decoder.SetPosition(1, pointR);
			decoder.Refresh();
			
			decoder.Process(&bformat, AUDIO_BUFFER_SIZE, stereoPtr);
			
			Assert(framesPerBuffer == AUDIO_BUFFER_SIZE);
			float * dst = (float*)outputBuffer;
			for (int i = 0; i < AUDIO_BUFFER_SIZE; ++i)
			{
				dst[i * 2 + 0] = stereo[0][i];
				dst[i * 2 + 1] = stereo[1][i];
			}
		}
		else
		{
			memset(outputBuffer, 0, framesPerBuffer * sizeof(float) * 2);
		}
	}
};

int main(int argc, char * argv[])
{
#ifdef DEBUG
	framework.enableRealTimeEditing = true;
#else
	const char * basePath = SDL_GetBasePath();
	changeDirectory(basePath);
	
	framework.fullscreen = true;
#endif
	
	if (!framework.init(0, nullptr, GFX_SX, GFX_SY))
		return -1;

	// create media player
	
	MediaPlayer * mp = new MediaPlayer();
	mp->openAsync("01.mp4", MP::kOutputMode_RGBA);

	//
	
	mouse.showCursor(false);
	mouse.setRelative(true);
	
	// initialize audio system
	
	MyAudioHandler audioHandler;
	audioHandler.init("sound.wav");
	
	PortAudioObject paObject;
	paObject.init(44100, 2, 0, AUDIO_BUFFER_SIZE, &audioHandler);
	
	for (;;)
	{
		framework.process();
		
		if (keyboard.wentDown(SDLK_ESCAPE))
			framework.quitRequested = true;
		
		if (framework.quitRequested)
			break;
		
		const float dt = std::min(1.f / 15.f, framework.timeStep);
		
		double azimuth = 0.0;
		
		SDL_LockMutex(s_mutex);
		{
			audioHandler.azimuthSpeed += mouse.dx / 10.0;
			
			azimuth = audioHandler.azimuth;
		}
		SDL_UnlockMutex(s_mutex);

		// update video player
		
		if (mp->presentedLastFrame(mp->context))
		{
			auto openParams = mp->context->openParams;
			
			mp->close(false);
			
			mp->presentTime = 0.0;
			
			mp->openAsync(openParams);
		}
		else
		{
			mp->presentTime += dt;
		}
		
		mp->tick(mp->context, true);

		framework.beginDraw(0, 0, 0, 0);
		{
		#if 1
			Shader shader("360video");
			setShader(shader);
			shader.setTexture("source", 0, mp->getTexture());
			shader.setImmediate("time", framework.time);
			shader.setImmediate("cam_azimuth", azimuth);
			
			const float time = framework.time;
			const float drift = (sinf(time * .123f) + 1.f) / 2.f;
			const float zoom = 1.f + (1.f + cosf(time * .234f)) / 2.f * .2f;
			
			shader.setImmediate("drift", drift);
			shader.setImmediate("zoom", zoom);

			{
				pushBlend(BLEND_OPAQUE);
				drawRect(0, 0, GFX_SX, GFX_SY);
				popBlend();
			}
			clearShader();
		#else
			gxSetTexture(mp->getTexture());
			{
				setColor(colorWhite);
				pushBlend(BLEND_OPAQUE);
				drawRect(0, 0, GFX_SX, GFX_SY);
				popBlend();
			}
			gxSetTexture(0);
		#endif
		}
		framework.endDraw();
	}
	
	paObject.shut();
	
	mp->close(true);
	
	delete mp;
	mp = nullptr;
	
	framework.shutdown();
	
	return 0;
}
