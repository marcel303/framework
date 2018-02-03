#include "framework.h"
#include "objects/audioSourceVorbis.h"
#include "objects/binauralizer.h"
#include "objects/binaural_cipic.h"
#include "objects/paobject.h"
#include "soundmix.h"
#include "video.h"
#include <atomic>

#define MAX_VOLUMES 6
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
	binaural::Mutex * mutex;
	
	binaural::Binauralizer binauralizer[MAX_BINAURALIZERS_PER_VOLUME];
	float gain[MAX_BINAURALIZERS_PER_VOLUME];
	
	AudioSource * source;
	
	ALIGN16 float samplesL[AUDIO_UPDATE_SIZE];
	ALIGN16 float samplesR[AUDIO_UPDATE_SIZE];
	
	MultiChannelAudioSource_SoundVolume()
		: mutex(nullptr)
		, binauralizer()
		, source(nullptr)
	{
		memset(gain, 0, sizeof(gain));
	}
	
	void init(const binaural::HRIRSampleSet * sampleSet, binaural::Mutex * _mutex)
	{
		mutex = _mutex;
		
		for (int i = 0; i < MAX_BINAURALIZERS_PER_VOLUME; ++i)
			binauralizer[i].init(sampleSet, _mutex);
	}
	
	void fillBuffers_naive(const int numSamples)
	{
		// generate source audio
		
		memset(samplesL, 0, sizeof(samplesL));
		memset(samplesR, 0, sizeof(samplesR));
	
		ALIGN16 float sourceBuffer[AUDIO_UPDATE_SIZE];
		source->generate(sourceBuffer, numSamples);
		
		// run binauralizers over the source audio
		
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
	
	void fillBuffers_optimized(const int numSamples)
	{
		// generate source audio
		
		memset(samplesL, 0, sizeof(samplesL));
		memset(samplesR, 0, sizeof(samplesR));
	
		ALIGN16 float sourceBuffer[AUDIO_UPDATE_SIZE];
		source->generate(sourceBuffer, numSamples);
		
		// fetch all of the HRIR sample datas we need based on the current elevation and azimuth pairs
		
		const binaural::HRIRSampleData * samples[3 * MAX_BINAURALIZERS_PER_VOLUME];
		float sampleWeights[3 * MAX_BINAURALIZERS_PER_VOLUME];
		float gainCopy[MAX_BINAURALIZERS_PER_VOLUME];
		
		mutex->lock();
		{
			memcpy(gainCopy, gain, sizeof(gainCopy));
			
			for (int i = 0; i < MAX_BINAURALIZERS_PER_VOLUME; ++i)
			{
				binauralizer[i].sampleSet->lookup_3(
					binauralizer[i].sampleLocation.elevation,
					binauralizer[i].sampleLocation.azimuth,
					samples + i * 3,
					sampleWeights + i * 3);
			}
		}
		mutex->unlock();
		
		// combine individual HRIRs into one combined HRIR
		
		binaural::HRIRSampleData combinedHrir;
		memset(&combinedHrir, 0, sizeof(combinedHrir));
		
		const binaural::HRIRSampleData ** samplesPtr = samples;
		float * sampleWeightsPtr = sampleWeights;
		
		for (int i = 0; i < MAX_BINAURALIZERS_PER_VOLUME; ++i)
		{
			const float gain = gainCopy[i];
			
			for (int j = 0; j < 3; ++j)
			{
				audioBufferAdd(combinedHrir.lSamples, samplesPtr[j]->lSamples, binaural::HRIR_BUFFER_SIZE, sampleWeightsPtr[j] * gain);
				audioBufferAdd(combinedHrir.rSamples, samplesPtr[j]->rSamples, binaural::HRIR_BUFFER_SIZE, sampleWeightsPtr[j] * gain);
			}
			
			samplesPtr += 3;
			sampleWeightsPtr += 3;
		}
		
		// run the binauralizer over the source audio
		
		binauralizer[0].provide(sourceBuffer, numSamples);
		
		binauralizer[0].generateLR(samplesL, samplesR, numSamples, &combinedHrir);
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
		
		//
		
		if (channelIndex == 0)
		{
			fillBuffers_optimized(numSamples);
		}
		
		//
		
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

struct Videoclip
{
	AudioSourceVorbis soundSource;
	SoundVolume soundVolume;
	MediaPlayer mp;
	float gain;
	
	void open(const char * audio, const char * video, const float _gain)
	{
		soundSource.open(audio, true);
		
		mp.openAsync(video, MP::kOutputMode_RGBA);
		
		gain = _gain;
	}
	
	void tick(const float dt)
	{
		mp.presentTime += dt;
		
		mp.tick(mp.context, true);
	}
	
	void drawSolid()
	{
		gxPushMatrix();
		{
			gxMultMatrixf(soundVolume.transform.m_v);
			
			gxSetTexture(mp.getTexture());
			{
				setColor(colorWhite);
				drawRect(-1, -1, +1, +1);
			}
			gxSetTexture(0);
		}
		gxPopMatrix();
		
		setColor(160, 160, 160);
		drawSoundVolume(soundVolume);
	}
	
	void drawTranslucent()
	{
	
	}
};

int main(int argc, char * argv[])
{
	if (!framework.init(0, nullptr, GFX_SX, GFX_SY))
		return -1;
	
	const int kFontSize = 16;
	const int kFontSize2 = 10;
	
	bool enableNearest = true;
	bool enableVertices = true;
	
	bool showUi = true;
	
	Camera3d camera;
	camera.gamepadIndex = 0;
	
	camera.position[0] = 0;
	camera.position[1] = +.3f;
	camera.position[2] = -1.f;
	camera.pitch = 10.f;
	
	float fov = 90.f;
	float near = .01f;
	float far = 100.f;
	
	SDL_mutex * audioMutex = SDL_CreateMutex();
	
	const int numFilenames = 3;
	
	const char * audioFilenames[numFilenames] =
	{
		"0.1.ogg",
		"1.1.ogg",
		"2.1.ogg",
	};
	
	const char * videoFilenames[numFilenames] =
	{
		"0.320px.mp4",
		"1.320px.mp4",
		"2.320px.mp4",
	};
	
	const float gains[numFilenames] =
	{
		1.f,
		.3f,
		1.f
	};
	
	Videoclip videoclips[MAX_VOLUMES];
	
	for (int i = 0; i < MAX_VOLUMES; ++i)
	{
		const int index = i % numFilenames;
		
		const char * audioFilename = audioFilenames[index];
		const char * videoFilename = videoFilenames[index];
		
		videoclips[i].open(audioFilename, videoFilename, gains[index]);
	}
	
	MyMutex binauralMutex(audioMutex);
	
	binaural::HRIRSampleSet sampleSet;
	binaural::loadHRIRSampleSet_Cipic("subject147", sampleSet);
	sampleSet.finalize();
	
	MyPortAudioHandler * paHandler = new MyPortAudioHandler();
	
	for (int i = 0; i < MAX_VOLUMES; ++i)
	{
		paHandler->audioSources[i].init(&sampleSet, &binauralMutex);
		
		paHandler->audioSources[i].source = &videoclips[i].soundSource;
	}
	
	PortAudioObject pa;
	pa.init(SAMPLE_RATE, 2, 0, AUDIO_UPDATE_SIZE, paHandler);
	
	float timeSeed = 1234.f;
	
	do
	{
		framework.process();

		const float dt = framework.timeStep;
		const float time = timeSeed + framework.time;
		//const float time = timeSeed + mouse.x;
		//const float time = timeSeed;
		
		// process input
		
		if (keyboard.wentDown(SDLK_t))
			timeSeed = random(0.f, 1000.f);
		
		if (keyboard.wentDown(SDLK_n))
			enableNearest = !enableNearest;
		
		if (keyboard.wentDown(SDLK_v))
			enableVertices = !enableVertices;
		
		if (keyboard.wentDown(SDLK_TAB))
			showUi = !showUi;
		
		// update the camera
		
		camera.tick(dt, true);
		
		// update video clips
		
		for (int i = 0; i < MAX_VOLUMES; ++i)
		{
			videoclips[i].tick(dt);
		}
		
		// update sound volume properties
		
		for (int i = 0; i < MAX_VOLUMES; ++i)
		{
			auto & soundVolume = videoclips[i].soundVolume;
			
			const float moveSpeed = (1.f + i / float(MAX_VOLUMES)) * .2f;
			const float moveAmount = 4.f / (i / float(MAX_VOLUMES) + 1);
			const float x = std::sin(moveSpeed * time / 11.234f) * moveAmount;
			const float y = std::sin(moveSpeed * time / 13.456f) * moveAmount;
			const float z = std::sin(moveSpeed * time / 15.678f) * moveAmount;
			
			const float scaleSpeed = 1.f + i / 5.f;
			const float scaleY = lerp(.5f, 1.f, (std::cos(scaleSpeed * time / 4.567f) + 1.f) / 2.f);
			const float scaleX = scaleY * 4.f/3.f;
			const float scaleZ = lerp(.05f, .5f, (std::cos(scaleSpeed * time / 8.765f) + 1.f) / 2.f);
			
			const float rotateSpeed = 1.f + i / 5.f;
			soundVolume.transform = Mat4x4(true).Translate(x, y, z).RotateX(rotateSpeed * time / 3.456f).RotateY(rotateSpeed * time / 4.567f).Scale(scaleX, scaleY, scaleZ);
		}
		
		// gather HRTF sampling points
		
		Vec3 samplePoints[MAX_BINAURALIZERS_TOTAL];
		Vec3 samplePointsView[MAX_BINAURALIZERS_TOTAL];
		float samplePointsAmount[MAX_BINAURALIZERS_TOTAL];
		int numSamplePoints = 0;
		
		const Vec3 pCameraWorld = camera.getWorldMatrix().GetTranslation();
		const Vec3 pSoundWorld = videoclips[0].soundVolume.transform.GetTranslation();
		const Vec3 pSoundView = camera.getViewMatrix().Mul4(pSoundWorld);
		
		for (int i = 0; i < MAX_VOLUMES; ++i)
		{
			auto & videoclip = videoclips[i];
			auto & soundVolume = videoclip.soundVolume;
			
			Vec3 svSamplePoints[MAX_BINAURALIZERS_PER_VOLUME];
			int numSvSamplePoints = 0;
			
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
					binaural::cartesianToElevationAndAzimuth(pView[2], pView[1], pView[0], elevation, azimuth);
					
					// morph to an elevation and azimuth of (0, 0) as the sound gets closer to the center of the head
					// perhaps we should add a dry-wet mix instead .. ?
					elevation = lerp(0.f, elevation, fadeAmount);
					azimuth = lerp(0.f, azimuth, fadeAmount);
					
					const float kMinDistanceToEar = .2f;
					const float clampedDistanceToEar = std::max(kMinDistanceToEar, distanceToHead);
					
					//const float gain = videoclip.gain / clampedDistanceToEar;
					const float gain = videoclip.gain / (clampedDistanceToEar * clampedDistanceToEar);
					
					paHandler->audioSources[i].binauralizer[j].setSampleLocation(elevation, azimuth);
					paHandler->audioSources[i].gain[j] = gain / numSvSamplePoints;
					
					//
					
					samplePoints[numSamplePoints] = pWorld;
					samplePointsView[numSamplePoints] = pView;
					samplePointsAmount[numSamplePoints] = fadeAmount;
					numSamplePoints++;
				}
				
				// reset and mute the unused binauralizers
				
				for (int j = numSvSamplePoints; j < MAX_BINAURALIZERS_PER_VOLUME; ++j)
				{
					paHandler->audioSources[i].binauralizer[j].setSampleLocation(0, 0);
					paHandler->audioSources[i].gain[j] = 0.f;
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
				glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
				glEnable(GL_LINE_SMOOTH);
				
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
						videoclips[i].drawSolid();
					}
				}
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
				drawText(10, 40, kFontSize, +1, +1, "N: toggle use nearest point (%s)", enableNearest ? "on" : "off");
				drawText(10, 60, kFontSize, +1, +1, "V: toggle use vertices (%s)", enableVertices ? "on" : "off");
			}
			
			popFontMode();
		}
		framework.endDraw();
	} while (!keyboard.wentDown(SDLK_ESCAPE));
	
	pa.shut();
	
	delete paHandler;
	paHandler = nullptr;
	
	SDL_DestroyMutex(audioMutex);
	audioMutex = nullptr;
	
	Font("calibri.ttf").saveCache();
	
	framework.shutdown();
	
	return 0;
}
