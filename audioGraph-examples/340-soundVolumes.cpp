//#include "audiostream/AudioStreamVorbis.h"
#include "framework.h"
#include "objects/audioSourceVorbis.h"
#include "objects/binauralizer.h"
#include "objects/binaural_cipic.h"
#include "objects/paobject.h"
#include "soundmix.h"

#define MAX_VOLUMES 2
#define MAX_BINAURALIZERS_PER_VOLUME 10 // center + nearest + eight vertices
#define MAX_BINAURALIZERS_TOTAL (MAX_BINAURALIZERS_PER_VOLUME * MAX_VOLUMES)

const int GFX_SX = 1024;
const int GFX_SY = 768;

struct MyMutex : binaural::Mutex
{
	SDL_mutex * mutex;
	
	MyMutex(SDL_mutex * _mutex)
		: mutex(_mutex)
	{
	}
	
	virtual void lock() override
	{
		SDL_LockMutex(mutex);
	}
	
	virtual void unlock() override
	{
		SDL_UnlockMutex(mutex);
	}
};

struct MultiChannelAudioSource
{
	virtual void generate(const int channelIndex, SAMPLE_ALIGN16 float * __restrict audioBuffer, const int numSamples) = 0;
};

struct MultiChannelAudioSource_SoundVolume : MultiChannelAudioSource
{
	binaural::Binauralizer binauralizer[MAX_BINAURALIZERS_PER_VOLUME];
	float gain[MAX_BINAURALIZERS_PER_VOLUME];
	
	AudioSource * source;
	
	ALIGN16 float samplesL[AUDIO_UPDATE_SIZE];
	ALIGN16 float samplesR[AUDIO_UPDATE_SIZE];
	
	MultiChannelAudioSource_SoundVolume()
		: binauralizer()
		, source(nullptr)
	{
		memset(gain, 0, sizeof(gain));
	}
	
	void init(const binaural::HRIRSampleSet * sampleSet, binaural::Mutex * mutex)
	{
		for (int i = 0; i < MAX_BINAURALIZERS_PER_VOLUME; ++i)
			binauralizer[i].init(sampleSet, mutex);
	}
	
	virtual void generate(const int channelIndex, SAMPLE_ALIGN16 float * __restrict audioBuffer, const int numSamples) override
	{
		Assert(channelIndex < 2);
		Assert(numSamples <= AUDIO_UPDATE_SIZE);
		
		if (source == nullptr)
		{
			memset(audioBuffer, 0, numSamples * sizeof(float));
			return;
		}
		
		if (channelIndex == 0)
		{
			memset(samplesL, 0, sizeof(samplesL));
			memset(samplesR, 0, sizeof(samplesR));
			
			ALIGN16 float sourceBuffer[AUDIO_UPDATE_SIZE];
			memset(sourceBuffer, 0xff, sizeof(sourceBuffer));
			
			source->generate(sourceBuffer, numSamples);
			
			for (int i = 0; i < MAX_BINAURALIZERS_PER_VOLUME; ++i)
			{
				binauralizer[i].provide(sourceBuffer, numSamples);
				
				ALIGN16 float tempL[AUDIO_UPDATE_SIZE];
				ALIGN16 float tempR[AUDIO_UPDATE_SIZE];
				
				float gainCopy;
				
				binauralizer[i].mutex->lock();
				{
					binauralizer[i].generateLR(tempL, tempR, numSamples);
					
					gainCopy = gain[i];
				}
				binauralizer[i].mutex->unlock();
				
				if (gainCopy != 0.f)
				{
					audioBufferAdd(samplesL, tempL, numSamples, gainCopy);
					audioBufferAdd(samplesR, tempR, numSamples, gainCopy);
				}
			}
		}
		
		if (channelIndex == 0)
		{
			audioBufferAdd(audioBuffer, samplesL, numSamples);
		}
		else
		{
			audioBufferAdd(audioBuffer, samplesR, numSamples);
		}
	}
};

struct SoundVolume
{
	Mat4x4 transform;
	
	SoundVolume()
		: transform(true)
	{
	}
	
	Vec3 projectToSound(Vec3Arg v) const
	{
		return transform.CalcInv().Mul4(v);
	}
	
	Vec3 projectToWorld(Vec3Arg v) const
	{
		return transform.Mul4(v);
	}
	
	Vec3 nearestPointWorld(Vec3Arg targetWorld) const
	{
		const Vec3 target = projectToSound(targetWorld);
		
		const float x = std::max(-1.f, std::min(+1.f, target[0]));
		const float y = std::max(-1.f, std::min(+1.f, target[1]));
		const float z = std::max(-1.f, std::min(+1.f, target[2]));
		
		return projectToWorld(Vec3(x, y, z));
	}
};

struct MyPortAudioHandler : PortAudioHandler
{
	MultiChannelAudioSource_SoundVolume audioSources[MAX_VOLUMES];
	
	MyPortAudioHandler()
		: PortAudioHandler()
		, audioSources()
	{
	}
	
	virtual void portAudioCallback(
		const void * inputBuffer,
		const int numInputChannels,
		void * outputBuffer,
		const int framesPerBuffer) override
	{
		ALIGN16 float channelL[AUDIO_UPDATE_SIZE];
		ALIGN16 float channelR[AUDIO_UPDATE_SIZE];
		
		memset(channelL, 0, sizeof(channelL));
		memset(channelR, 0, sizeof(channelR));
		
		for (int i = 0; i < MAX_VOLUMES; ++i)
		{
			audioSources[i].generate(0, channelL, AUDIO_UPDATE_SIZE);
			audioSources[i].generate(1, channelR, AUDIO_UPDATE_SIZE);
		}
		
		float * __restrict destinationBuffer = (float*)outputBuffer;
		
		for (int i = 0; i < AUDIO_UPDATE_SIZE; ++i)
		{
			destinationBuffer[i * 2 + 0] = channelL[i];
			destinationBuffer[i * 2 + 1] = channelR[i];
		}
	}
};

static void drawSoundVolume(const SoundVolume & volume)
{
	gxPushMatrix();
	{
		gxMultMatrixf(volume.transform.m_v);
		
		gxPushMatrix(); { gxTranslatef(-1, 0, 0); drawGrid3dLine(10, 10, 1, 2); } gxPopMatrix();
		gxPushMatrix(); { gxTranslatef(+1, 0, 0); drawGrid3dLine(10, 10, 1, 2); } gxPopMatrix();
		gxPushMatrix(); { gxTranslatef(0, -1, 0); drawGrid3dLine(10, 10, 2, 0); } gxPopMatrix();
		gxPushMatrix(); { gxTranslatef(0, +1, 0); drawGrid3dLine(10, 10, 2, 0); } gxPopMatrix();
		gxPushMatrix(); { gxTranslatef(0, 0, -1); drawGrid3dLine(10, 10, 0, 1); } gxPopMatrix();
		gxPushMatrix(); { gxTranslatef(0, 0, +1); drawGrid3dLine(10, 10, 0, 1); } gxPopMatrix();
	}
	gxPopMatrix();
}

static void drawSoundVolume_Translucent(const SoundVolume & volume)
{
	gxPushMatrix();
	{
		gxMultMatrixf(volume.transform.m_v);
		
		gxPushMatrix(); { gxTranslatef(-1, 0, 0); drawRect3d(1, 2); } gxPopMatrix();
		gxPushMatrix(); { gxTranslatef(+1, 0, 0); drawRect3d(1, 2); } gxPopMatrix();
		gxPushMatrix(); { gxTranslatef(0, -1, 0); drawRect3d(2, 0); } gxPopMatrix();
		gxPushMatrix(); { gxTranslatef(0, +1, 0); drawRect3d(2, 0); } gxPopMatrix();
		gxPushMatrix(); { gxTranslatef(0, 0, -1); drawRect3d(0, 1); } gxPopMatrix();
		gxPushMatrix(); { gxTranslatef(0, 0, +1); drawRect3d(0, 1); } gxPopMatrix();
	}
	gxPopMatrix();
}

static void drawPoint(Vec3Arg p, const Color & c1, const Color & c2, const Color & c3, const float size)
{
	gxBegin(GL_LINES);
	{
		setColor(c1);
		gxVertex3f(p[0]-size, p[1], p[2]);
		gxVertex3f(p[0]+size, p[1], p[2]);
		
		setColor(c2);
		gxVertex3f(p[0], p[1]-size, p[2]);
		gxVertex3f(p[0], p[1]+size, p[2]);
		
		setColor(c3);
		gxVertex3f(p[0], p[1], p[2]-size);
		gxVertex3f(p[0], p[1], p[2]+size);
	}
	gxEnd();
}

static const Vec3 s_cubeVertices[8] =
{
	Vec3(-1, -1, -1),
	Vec3(+1, -1, -1),
	Vec3(+1, +1, -1),
	Vec3(-1, +1, -1),
	Vec3(-1, -1, +1),
	Vec3(+1, -1, +1),
	Vec3(+1, +1, +1),
	Vec3(-1, +1, +1)
};

int main(int argc, char * argv[])
{
	if (!framework.init(0, nullptr, GFX_SX, GFX_SY))
		return -1;
	
	const int kFontSize = 16;
	const int kFontSize2 = 10;
	
	bool enableDistanceAttenuation = true;
	
	bool enableCenter = false;
	bool enableNearest = true;
	bool enableVertices = false;
	
	bool flipUpDown = false;
	
	bool showUi = true;
	
	Camera3d camera;
	
	camera.position[0] = 0;
	camera.position[1] = +.3f;
	camera.position[2] = -1.f;
	camera.pitch = 10.f;
	
	float fov = 90.f;
	float near = .01f;
	float far = 100.f;
	
	SDL_mutex * audioMutex = SDL_CreateMutex();
	
	const char * filenames[] =
	{
		"wobbly.ogg",
		"hrtf/music2.ogg",
		"hrtf/music.ogg"
	};
	const int numFilenames = sizeof(filenames) / sizeof(filenames[0]);
	
	AudioSourceVorbis oggSources[MAX_VOLUMES];
	
	for (int i = 0; i < MAX_VOLUMES; ++i)
	{
		const char * filename = filenames[i % numFilenames];
		
		oggSources[i].open(filename, true);
	}
	
	MyMutex binauralMutex(audioMutex);
	
	binaural::HRIRSampleSet sampleSet;
	binaural::loadHRIRSampleSet_Cipic("hrtf/CIPIC/subject147", sampleSet);
	sampleSet.finalize();
	
	MyPortAudioHandler paHandler;
	
	for (int i = 0; i < MAX_VOLUMES; ++i)
	{
		paHandler.audioSources[i].init(&sampleSet, &binauralMutex);
		
		paHandler.audioSources[i].source = &oggSources[i];
	}
	
	PortAudioObject pa;
	pa.init(SAMPLE_RATE, 2, 0, AUDIO_UPDATE_SIZE, &paHandler);
	
	SoundVolume soundVolumes[MAX_VOLUMES];
	
	do
	{
		framework.process();

		const float dt = framework.timeStep;
		
		// process input
		
		if (keyboard.wentDown(SDLK_a))
			enableDistanceAttenuation = !enableDistanceAttenuation;
		
		if (keyboard.wentDown(SDLK_c))
		{
			enableCenter = !enableCenter;
			enableNearest = false;
		}
		
		if (keyboard.wentDown(SDLK_n))
		{
			enableNearest = !enableNearest;
			enableCenter = false;
		}
		
		if (keyboard.wentDown(SDLK_v))
			enableVertices = !enableVertices;
		
		if (keyboard.wentDown(SDLK_f))
			flipUpDown = !flipUpDown;
		
		if (keyboard.wentDown(SDLK_TAB))
			showUi = !showUi;
		
		// update the camera
		
		camera.tick(dt, true);
		
		// update sound volume properties
		
		for (int i = 0; i < MAX_VOLUMES; ++i)
		{
			auto & soundVolume = soundVolumes[i];
			
			const float moveSpeed = 5.f + i / 5.f;
			const float moveAmount = 4.f / (i + 1);
			const float x = std::sin(moveSpeed * framework.time / 11.234f) * moveAmount;
			const float y = std::sin(moveSpeed * framework.time / 13.456f) * moveAmount;
			const float z = std::sin(moveSpeed * framework.time / 15.678f) * moveAmount;
			
			const float scaleSpeed = 1.f + i / 5.f;
			const float scaleX = lerp(.5f, 1.f, (std::cos(scaleSpeed * framework.time / 4.567f) + 1.f) / 2.f);
			const float scaleY = scaleX * 1.5f;
			const float scaleZ = lerp(.05f, .5f, (std::cos(scaleSpeed * framework.time / 8.765f) + 1.f) / 2.f);
			
			const float rotateSpeed = 1.f + i / 5.f;
			soundVolume.transform = Mat4x4(true).Translate(x, y, z).RotateX(rotateSpeed * framework.time / 3.456f).RotateY(rotateSpeed * framework.time / 4.567f).Scale(scaleX, scaleY, scaleZ);
		}
		
		// gather HRTF sampling points
		
		Vec3 samplePoints[MAX_BINAURALIZERS_TOTAL];
		Vec3 samplePointsView[MAX_BINAURALIZERS_TOTAL];
		float samplePointsAmount[MAX_BINAURALIZERS_TOTAL];
		int numSamplePoints = 0;
		
		const Vec3 pCameraWorld = camera.getWorldMatrix().GetTranslation();
		const Vec3 pSoundWorld = soundVolumes[0].transform.GetTranslation();
		const Vec3 pSoundView = camera.getViewMatrix().Mul4(pSoundWorld);
		
		for (int i = 0; i < MAX_VOLUMES; ++i)
		{
			auto & soundVolume = soundVolumes[i];
			
			Vec3 svSamplePoints[MAX_BINAURALIZERS_PER_VOLUME];
			int numSvSamplePoints = 0;
			
			if (enableCenter)
			{
				svSamplePoints[numSvSamplePoints++] = soundVolume.transform.GetTranslation();
			}
			
			if (enableNearest)
			{
				const Vec3 nearestPointWorld = soundVolume.nearestPointWorld(pCameraWorld);
				
				svSamplePoints[numSvSamplePoints++] = nearestPointWorld;
			}
			
			if (enableVertices)
			{
				for (int j = 0; j < 8; ++j)
				{
					const Vec3 pWorld = soundVolume.projectToWorld(s_cubeVertices[j]);
					
					svSamplePoints[numSvSamplePoints++] = pWorld;
				}
			}
			
			binauralMutex.lock();
			{
				// activate the binauralizers for the generated sample points
				
				for (int j = 0; j < numSvSamplePoints; ++j)
				{
					const Vec3 & pWorld = svSamplePoints[j];
					const Vec3 pView = camera.getViewMatrix().Mul4(pWorld);
					
					const float distanceToHead = pView.CalcSize();
					const float kDistanceToHeadTreshold = .1f; // 10cm. related to head size, but exact size is subjective
					
					const float fadeAmount = std::min(1.f, distanceToHead / kDistanceToHeadTreshold);
					
					float elevation;
					float azimuth;
					binaural::cartesianToElevationAndAzimuth(pView[2], flipUpDown ? -pView[1] : +pView[1], pView[0], elevation, azimuth);
					
					// morph to an elevation and azimuth of (0, 0) as the sound gets closer to the center of the head
					// perhaps we should add a dry-wet mix instead .. ?
					elevation = lerp(0.f, elevation, fadeAmount);
					azimuth = lerp(0.f, azimuth, fadeAmount);
					
					const float kMinDistanceToEar = .2f;
					const float clampedDistanceToEar = std::max(kMinDistanceToEar, distanceToHead);
					
					//const float gain = enableDistanceAttenuation ? .1f / (clampedDistanceToEar * clampedDistanceToEar) : 1.f;
					const float gain = enableDistanceAttenuation ? .5f / clampedDistanceToEar : 1.f;
					
					paHandler.audioSources[i].binauralizer[j].setSampleLocation(elevation, azimuth);
					paHandler.audioSources[i].gain[j] = gain / numSvSamplePoints;
					
					samplePoints[numSamplePoints] = pWorld;
					samplePointsView[numSamplePoints] = pView;
					samplePointsAmount[numSamplePoints] = fadeAmount;
					numSamplePoints++;
				}
				
				// reset and mute the unused binauralizers
				
				for (int j = numSvSamplePoints; j < MAX_BINAURALIZERS_PER_VOLUME; ++j)
				{
					paHandler.audioSources[i].binauralizer[j].setSampleLocation(0, 0);
					paHandler.audioSources[i].gain[j] = 0.f;
				}
			}
			binauralMutex.unlock();
		}
		
		Assert(numSamplePoints <= MAX_BINAURALIZERS_TOTAL);
		
		framework.beginDraw(0, 0, 0, 0);
		{
			setFont("calibri.ttf");
			pushFontMode(FONT_SDF);
			
			projectPerspective3d(fov, near, far);
			
			camera.pushViewMatrix();
			{
				glEnable(GL_DEPTH_TEST);
				glDepthFunc(GL_LESS);
				{
					gxPushMatrix();
					{
						gxScalef(10, 10, 10);
						setColor(50, 50, 50);
						drawGrid3dLine(100, 100, 0, 2, true);
					}
					gxPopMatrix();
					
					for (int i = 0; i < MAX_VOLUMES; ++i)
					{
						setColor(160, 160, 160);
						drawSoundVolume(soundVolumes[i]);
					}
					
					for (int i = 0; i < numSamplePoints; ++i)
					{
						drawPoint(samplePoints[i], colorRed, colorGreen, colorBlue, .1f);
					}
				}
				glDisable(GL_DEPTH_TEST);
				
				//
				
				glEnable(GL_DEPTH_TEST);
				glDepthFunc(GL_LESS);
				glDepthMask(GL_FALSE);
				pushBlend(BLEND_ADD);
				{
					for (int i = 0; i < MAX_VOLUMES; ++i)
					{
						setColor(60, 100, 160, 63);
						
						drawSoundVolume_Translucent(soundVolumes[i]);
					}
				}
				popBlend();
				glDepthMask(GL_TRUE);
				glDisable(GL_DEPTH_TEST);
			}
			camera.popViewMatrix();
			
			projectScreen2d();
			
			if (showUi)
			{
				setColor(colorWhite);
				drawText(10, 10, kFontSize, +1, +1, "sound world pos: %.2f, %.2f, %.2f", pSoundWorld[0], pSoundWorld[1], pSoundWorld[2]);
				drawText(10, 30, kFontSize, +1, +1, "sound view pos: %.2f, %.2f, %.2f", pSoundView[0], pSoundView[1], pSoundView[2]);
				drawText(10, 50, kFontSize, +1, +1, "camera world pos: %.2f, %.2f, %.2f", pCameraWorld[0], pCameraWorld[1], pCameraWorld[2]);
				
				beginTextBatch();
				{
					int x = 10;
					int y = 100;
					
					for (int i = 0; i < numSamplePoints; ++i)
					{
						setColor(200, 200, 200);
						drawText(x, y, kFontSize2, +1, +1,
							"sample pos: (%+.2f, %+.2f, %+.2f, world), (%+.2f, %+.2f, %+.2f, view), amount: %.2f",
							samplePoints[i][0], samplePoints[i][1], samplePoints[i][2],
							samplePointsView[i][0], samplePointsView[i][1], samplePointsView[i][2],
							samplePointsAmount[i]);
						
						y += 12;
						
						if (((i + 1) % 46) == 0)
						{
							x += 400;
							y = 100;
						}
					}
				}
				endTextBatch();
				
				gxTranslatef(0, GFX_SY - 100, 0);
				setColor(colorWhite);
				drawText(10, 0, kFontSize, +1, +1, "A: toggle distance attenuation (%s)", enableDistanceAttenuation ? "on" : "off");
				drawText(10, 20, kFontSize, +1, +1, "C: toggle use center point (%s)", enableCenter ? "on" : "off");
				drawText(10, 40, kFontSize, +1, +1, "N: toggle use nearest point (%s)", enableNearest ? "on" : "off");
				drawText(10, 60, kFontSize, +1, +1, "V: toggle use vertices (%s)", enableVertices ? "on" : "off");
				drawText(10, 80, kFontSize, +1, +1, "F: flip up-down axis for HRTF (%s)", flipUpDown ? "on" : "off");
			}
			
			popFontMode();
		}
		framework.endDraw();
	} while (!keyboard.wentDown(SDLK_ESCAPE));
	
	pa.shut();
	
	SDL_DestroyMutex(audioMutex);
	audioMutex = nullptr;
	
	framework.shutdown();
	
	return 0;
}
