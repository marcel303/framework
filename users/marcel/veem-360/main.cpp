#include "audio.h"
#include "framework.h"
#include "objects/paobject.h"
#include "video.h"
#include <algorithm>

#include "AmbisonicDecoder.h"

const int GFX_SX = 512 * 3/2;
const int GFX_SY = 512;

#define AUDIO_BUFFER_SIZE 64

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
		soundData = loadSound(filename);
		
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
				azimuthSpeed = +.7f;
			if (keyboard.isDown(SDLK_RIGHT))
				azimuthSpeed = -.7f;
			
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
#if defined(CHIBI_RESOURCE_PATH)
	changeDirectory(CHIBI_RESOURCE_PATH);
#else
	const char * basePath = SDL_GetBasePath();
	changeDirectory(basePath);
#endif

#ifdef DEBUG
	framework.enableRealTimeEditing = true;
#else
	framework.fullscreen = true;
#endif
	
	if (!framework.init(GFX_SX, GFX_SY))
		return -1;
	pushFontMode(FONT_SDF);
	
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
	
	const float moveSpeed = .2f;
	Camera3d cam;
	cam.mouseRotationSpeed = .4f;
	cam.maxForwardSpeed = moveSpeed;
	cam.maxUpSpeed = moveSpeed;
	cam.maxStrafeSpeed = moveSpeed;
	
	float fadeTimer = 7.f;
	//float fadeTimer = 1.f;
	
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
			audioHandler.azimuthSpeed -= mouse.dx / 10.0;
			
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

		cam.tick(dt, true);
		
		cam.yaw = azimuth * 180.f / M_PI;
		
		if (cam.position[1] > -.2f)
			cam.position[1] = -.2f;
		
		const float retainPerSecond = .9f;
		const float retain = powf(retainPerSecond, dt);
		Vec3 origin = Vec3(0.f, -.7f, 0.f);
		float originalPitch = -10.f;
		cam.position = lerp(origin, cam.position, retain);
		cam.pitch = lerp(originalPitch, cam.pitch, retain);
		
		if (cam.position.CalcSize() > .8f)
		{
			cam.position = cam.position.CalcNormalized() * .8f;
		}
		
		cam.pitch = clamp(cam.pitch, -80.f, -3.f);
		
		fadeTimer  = fmaxf(0.f, fadeTimer - dt);
		
		framework.beginDraw(0, 0, 0, 0);
		{
		#if 0
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
		#elif 0
			gxSetTexture(mp->getTexture());
			{
				setColor(colorWhite);
				pushBlend(BLEND_OPAQUE);
				drawRect(0, 0, GFX_SX, GFX_SY);
				popBlend();
			}
			gxSetTexture(0);
		#elif 0
			pushBlend(BLEND_OPAQUE);
			projectPerspective3d(60.f, .001f, 10.f);
			cam.pushViewMatrix();
			
			setColor(colorWhite);
			gxSetTexture(mp->getTexture());
			glPointSize(10.f);
			gxBegin(GL_POINTS);
			for (float u = -1.f; u <= +1.f; u += 1.f / 200.f)
			{
				for (float v = -1.f; v <= +1.f; v += 1.f / 200.f)
				{
					const float s = u * u + v * v;
					
					if (s <= 1.f)
					{
						const float h = sqrtf(1.f - s);
						
						gxTexCoord2f((u + 1.f) / 2.f, (v + 1.f) / 2.f);
						gxVertex3f(-u, -h, v);
					}
				}
			}
			gxEnd();
			gxSetTexture(0);
		
			cam.popViewMatrix();
			projectScreen2d();
			popBlend();
		#else
			pushBlend(BLEND_OPAQUE);
			projectPerspective3d(46.f, .001f, 10.f);
			cam.pushViewMatrix();
			
			Shader shader("360-points");
			setShader(shader);
			shader.setTexture("source", 0, mp->getTexture());
			drawGrid3d(100, 100);
			clearShader();
			
			cam.popViewMatrix();
			projectScreen2d();
			popBlend();
		#endif
		
			const float opacity = fminf(1.f, fadeTimer / 3.f);
			
			if (opacity > 0.f)
			{
				pushBlend(BLEND_ALPHA);
				setColorf(0.f, 0.f, 0.f, opacity);
				drawRect(0, 0, GFX_SX, GFX_SY);
				setColorf(.8f, .8f, .8f, opacity);
				setFont("calibri.ttf");
				drawText(GFX_SX/2, GFX_SY/2, 32, 0, 0, "Veem 360");
				setColorf(.8f, .8f, .8f, opacity * .7f);
				drawTextArea(GFX_SX/6, GFX_SY*5/6, GFX_SX*4/6, GFX_SY*1/6, 16, 0, 0, "Code & design © 2018 Marcel Smit\nhttps://http://centuryofthecat.nl/");
				popBlend();
			}
		
		#ifdef DEBUG
			setColor(200, 200, 200);
			setFont("calibri.ttf");
			drawText(10, 10, 14, +1, +1, "pitch: %.2f", cam.pitch);
		#endif
		}
		framework.endDraw();
	}
	
	paObject.shut();
	
	mp->close(true);
	
	delete mp;
	mp = nullptr;
	
	Font("calibri.ttf").saveCache();
	
	popFontMode();
	framework.shutdown();
	
	return 0;
}
