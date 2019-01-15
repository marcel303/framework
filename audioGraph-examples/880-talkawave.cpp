#include "framework.h"
#include "objects/audioSourceVorbis.h"
#include "objects/binauralizer.h"
#include "objects/binaural_cipic.h"
#include "objects/paobject.h"
#include "soundmix.h"
#include <cmath>
#include <deque>

//#define MAX_SOUNDVOLUMES 100
#define MAX_SOUNDVOLUMES 40
#define NUM_SPEEDERS 0

#define DO_RECORDING 1
#define NUM_BUFFERS_PER_RECORDING (2 * 44100 / AUDIO_UPDATE_SIZE)

#define DO_WATER 1
#define WATER_HEIGHT .1f

const int GFX_SX = 1280;
const int GFX_SY = 720;

struct RecordedData;

struct MyMutex : binaural::Mutex
{
	SDL_mutex * mutex;
	
	MyMutex(SDL_mutex * _mutex)
		: mutex(_mutex)
	{
	}
	
	virtual void lock() override
	{
		const int r = SDL_LockMutex(mutex);
		Verify(r == 0);
	}
	
	virtual void unlock() override
	{
		const int r = SDL_UnlockMutex(mutex);
		Verify(r == 0);
	}
};

static MyMutex * s_binauralMutex = nullptr;

static binaural::HRIRSampleSet * s_sampleSet = nullptr;

static Mat4x4 s_worldToViewTransform(true);
static Mat4x4 s_viewToWorldTransform(true);

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

struct SoundVolume
{
	static const int kMaxSamplePositions = 10;

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

	void generateSamplePositions(Vec3 * samplePositions, int & numSamplePositions)
	{
		const bool enableCenter = false;
		const bool enableNearest = true;
		const bool enableVertices = false;

		numSamplePositions = 0;
		
		const Vec3 cameraPosition_world = s_viewToWorldTransform.GetTranslation();
		const Vec3 soundPosition_world = transform.GetTranslation();
		
		if (enableCenter)
		{
			samplePositions[numSamplePositions++] = soundPosition_world;
		}
		
		if (enableNearest)
		{
			const Vec3 nearestPositions_world = nearestPointWorld(cameraPosition_world);
			
			samplePositions[numSamplePositions++] = nearestPositions_world;
		}
		
		if (enableVertices)
		{
			for (int i = 0; i < 8; ++i)
			{
				const Vec3 position_world = projectToWorld(s_cubeVertices[i]);
				
				samplePositions[numSamplePositions++] = position_world;
			}
		}
	}
};

struct MultiChannelAudioSource
{
	virtual void generate(const int channelIndex, SAMPLE_ALIGN16 float * __restrict audioBuffer, const int numSamples) = 0;
};

struct MultiChannelAudioSource_SoundVolume : MultiChannelAudioSource
{
	binaural::Mutex * mutex;

	SoundVolume soundVolume;
	
	AudioSource * source;
	
	binaural::Binauralizer binauralizer;
	
	ALIGN16 float samplesL[AUDIO_UPDATE_SIZE];
	ALIGN16 float samplesR[AUDIO_UPDATE_SIZE];
	
	MultiChannelAudioSource_SoundVolume()
		: mutex(nullptr)
		, source(nullptr)
		, binauralizer()
	{
	}
	
	void init(AudioSource * _source)
	{
		source = _source;
	}
	
	void fillBuffers(const int numSamples)
	{
		const bool enableDistanceAttenuation = true;
		
		// generate source audio
		
		memset(samplesL, 0, sizeof(samplesL));
		memset(samplesR, 0, sizeof(samplesR));
	
		ALIGN16 float sourceBuffer[AUDIO_UPDATE_SIZE];
		source->generate(sourceBuffer, numSamples);

		// and provide it to the binauralizer

		binauralizer.provide(sourceBuffer, numSamples);
		
		// get the HRIR sample positions from the sound volume
		
		Vec3 samplePositions[SoundVolume::kMaxSamplePositions];
		int numSamplePositions;

		soundVolume.generateSamplePositions(samplePositions, numSamplePositions);

		// fetch all of the HRIR sample datas we need based on the sample positions we get from the sound volume
		// and combine these individual HRIRs into one combined HRIR

		binaural::HRIRSampleData combinedHrir;
		memset(&combinedHrir, 0, sizeof(combinedHrir));

		for (int i = 0; i < numSamplePositions; ++i)
		{
			const Vec3 & position_world = samplePositions[i];
			const Vec3 position_view = s_worldToViewTransform.Mul4(position_world);
			
			const float distanceToHead = position_view.CalcSize();
			const float kDistanceToHeadTreshold = .1f; // 10cm. related to head size, but exact size is subjective
			
			const float fadeAmount = std::min(1.f, distanceToHead / kDistanceToHeadTreshold);
			
			float elevation;
			float azimuth;
			binaural::cartesianToElevationAndAzimuth(
				position_view[2],
				position_view[1],
				position_view[0],
				elevation,
				azimuth);
			
			// morph to an elevation and azimuth of (0, 0) as the sound gets closer to the center of the head
			// perhaps we should add a dry-wet mix instead .. ?
			elevation = lerp(0.f, elevation, fadeAmount);
			azimuth = lerp(0.f, azimuth, fadeAmount);
			
			const float kMinDistanceToEar = .2f;
			const float clampedDistanceToEar = std::max(kMinDistanceToEar, distanceToHead);
			
			const float attenuation = enableDistanceAttenuation ? .2f / (clampedDistanceToEar * clampedDistanceToEar) : 1.f;
			//const float attenuation = enableDistanceAttenuation ? .5f / clampedDistanceToEar : 1.f;
			const float gain = attenuation / numSamplePositions / 10.f;

			const binaural::HRIRSampleData * samples[3];
			float sampleWeights[3];

			if (s_sampleSet->lookup_3(
				elevation,
				azimuth,
				samples,
				sampleWeights))
			{
				for (int j = 0; j < 3; ++j)
				{
					audioBufferAdd(combinedHrir.lSamples, samples[j]->lSamples, binaural::HRIR_BUFFER_SIZE, sampleWeights[j] * gain);
					audioBufferAdd(combinedHrir.rSamples, samples[j]->rSamples, binaural::HRIR_BUFFER_SIZE, sampleWeights[j] * gain);
				}
			}
		}

		// run the binauralizer over the source audio
		
		binauralizer.generateLR(samplesL, samplesR, numSamples, &combinedHrir);
	}
	
	virtual void generate(const int channelIndex, SAMPLE_ALIGN16 float * __restrict audioBuffer, const int numSamples) override
	{
		Assert(channelIndex < 2);
		Assert(numSamples <= AUDIO_UPDATE_SIZE);
		
		if (source == nullptr)
		{
			return;
		}
		
		//
		
		if (channelIndex == 0)
		{
			fillBuffers(numSamples);
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

static float s_monoData[AUDIO_UPDATE_SIZE];

struct AudioSourceMonoData : AudioSource
{
	virtual void generate(SAMPLE_ALIGN16 float * __restrict samples, const int numSamples) override
	{
		memcpy(samples, s_monoData, numSamples * sizeof(float));
	}
};

static float s_inputData[AUDIO_UPDATE_SIZE];

static void tickAudio(const float dt);
static void addRecordedFragment(RecordedData & recordedData);
static float sampleHeight(Vec3Arg p);

struct MyPortAudioHandler : PortAudioHandler
{
	Mat4x4 worldToViewTransform;

	std::vector<MultiChannelAudioSource_SoundVolume*> audioSources;
	
	AudioSourceVorbis monoSource;
	
	MyPortAudioHandler()
		: PortAudioHandler()
		, worldToViewTransform(true)
		, audioSources()
		, monoSource()
	{
		monoSource.open("thegrooop/talkative.ogg", true);
	}

	void addAudioSource(MultiChannelAudioSource_SoundVolume * audioSource)
	{
		s_binauralMutex->lock();
		{
			audioSources.push_back(audioSource);
		}
		s_binauralMutex->unlock();
	}
	
	void removeAudioSource(MultiChannelAudioSource_SoundVolume * audioSource)
	{
		s_binauralMutex->lock();
		{
			auto i = std::find(audioSources.begin(), audioSources.end(), audioSource);
			
			if (i != audioSources.end())
				audioSources.erase(i);
		}
		s_binauralMutex->unlock();
	}
	
	virtual void portAudioCallback(
		const void * inputBuffer,
		const int numInputChannels,
		void * outputBuffer,
		const int framesPerBuffer) override
	{
		Assert(framesPerBuffer == AUDIO_UPDATE_SIZE);
		
		ALIGN16 float channelL[AUDIO_UPDATE_SIZE];
		ALIGN16 float channelR[AUDIO_UPDATE_SIZE];
		
		memset(channelL, 0, sizeof(channelL));
		memset(channelR, 0, sizeof(channelR));
		
		monoSource.generate(s_monoData, AUDIO_UPDATE_SIZE);
		
	#if 1
		if (numInputChannels > 0)
		{
			const float * input = (float*)inputBuffer;
			
			for (int i = 0; i < AUDIO_UPDATE_SIZE; ++i)
				s_inputData[i] = input[i * numInputChannels];
		}
		else
		{
			memset(s_inputData, 0, sizeof(s_inputData));
		}
	#else
		memcpy(s_inputData, s_monoData, sizeof(s_inputData));
	#endif
		
		s_binauralMutex->lock();
		{
			s_worldToViewTransform = worldToViewTransform;
			s_viewToWorldTransform = worldToViewTransform.CalcInv();
			
			tickAudio(AUDIO_UPDATE_SIZE / float(SAMPLE_RATE));
		
			for (auto & audioSource : audioSources)
			{
				audioSource->generate(0, channelL, AUDIO_UPDATE_SIZE);
				audioSource->generate(1, channelR, AUDIO_UPDATE_SIZE);
			}
		}
		s_binauralMutex->unlock();
		
		float * __restrict destinationBuffer = (float*)outputBuffer;
		
		for (int i = 0; i < AUDIO_UPDATE_SIZE; ++i)
		{
			destinationBuffer[i * 2 + 0] = channelL[i];
			destinationBuffer[i * 2 + 1] = channelR[i];
		}
	}
};

static MyPortAudioHandler * s_paHandler = nullptr;

#if DO_WATER

struct Waves
{
	static const int kSize = 128;
	
	Mat4x4 transform;
	
	float p[kSize][kSize];
	float v[kSize][kSize];
	
	Waves()
	{
		transform = Mat4x4(true)
			.Translate(0, -.05f, 0)
			.RotateX(M_PI/2.f)
			.Scale(10, 10, WATER_HEIGHT)
			.Translate(-.5f, -.5f, 0)
			.Scale(1.f / kSize, 1.f / kSize, 1)
			.Translate(.5f, .5f, 0);
		
		memset(p, 0, sizeof(p));
		memset(v, 0, sizeof(v));
	}
	
	Vec3 projectToWaves(Vec3Arg p) const
	{
		return transform.CalcInv().Mul4(p);
	}
	
	float sample(const float x, const float y) const
	{
		int x0 = int(floor(x));
		int y0 = int(floor(y));
		int x1 = int(ceil(x));
		int y1 = int(ceil(y));
		
		x0 = clamp(x0, 0, kSize - 1);
		y0 = clamp(y0, 0, kSize - 1);
		x1 = clamp(x1, 0, kSize - 1);
		y1 = clamp(y1, 0, kSize - 1);
		
		const float tx = x - x0;
		const float ty = y - y0;
		
		const float v00 = p[x0][y0];
		const float v10 = p[x1][y0];
		const float v01 = p[x0][y1];
		const float v11 = p[x1][y1];
		
		const float v0 = lerp(v00, v10, tx);
		const float v1 = lerp(v01, v11, tx);
		
		const float v = lerp(v0, v1, ty);
		
		return v;
	}
	
	void tick(const float dt)
	{
		const float stiffness = 100.f;
		const float falloff = std::pow(.6f, dt);
		
		if (mouse.isDown(BUTTON_LEFT))
		{
			p[kSize*1/3][kSize*1/3] = -.4f;
			p[kSize*2/3][kSize*1/3] = -.2f;
			p[kSize*2/3][kSize*2/3] = -.4f;
		}
		else
		{
			p[kSize/2][kSize/2] = 2.f;
		}
		
		// integrate forces into velocity
		
		for (int x = 0; x < kSize; ++x)
		{
			for (int y = 0; y < kSize; ++y)
			{
				const int x0 = x > 0 ? x - 1 : 0;
				const int y0 = y > 0 ? y - 1 : 0;
				const int x1 = x;
				const int y1 = y;
				const int x2 = x < kSize - 1 ? x + 1 : kSize - 1;
				const int y2 = y < kSize - 1 ? y + 1 : kSize - 1;
				
				float d = 0.f;
				
				d += p[x0][y1];
				d += p[x2][y1];
				d += p[x1][y0];
				d += p[x1][y2];
				
				d -= p[x1][y1] * 4.f;
				d /= 4.f;
				
				v[x1][y1] += d * stiffness * dt;
				v[x1][y1] *= falloff;
			}
		}
		
		// integrate velocity into position
		
		const float pFalloff = std::pow(.9f, dt);
		
		for (int x = 0; x < kSize; ++x)
		{
			for (int y = 0; y < kSize; ++y)
			{
				p[x][y] *= pFalloff;
				
				p[x][y] += v[x][y] * dt;
			}
		}
	}
};

#endif

const float kInitSpeederDistance1 = 8.f;
const float kInitSpeederDistance2 = 10.f;
const float kMaxSpeederDistance = 12.f;

struct Speeder : AudioSource
{
	Vec3 p;
	Vec3 v;
	
	float gain;
	bool rampUp;
	float rampUpVolume;
	
	AudioSourceMonoData monoData;
	
	MultiChannelAudioSource_SoundVolume audioSource;
	
	Speeder()
		: p()
		, v()
		, gain(0.f)
		, rampUp(false)
		, rampUpVolume(0.f)
		, monoData()
		, audioSource()
	{
	
	}
	
	void init()
	{
		audioSource.init(this);
		
		randomizePosition();
		updateTransform();
		
		s_paHandler->addAudioSource(&audioSource);
	}
	
	void randomizePosition()
	{
		const float angle = random(0.f, float(M_PI) * 2.f);
		const float distance = random(kInitSpeederDistance1, kInitSpeederDistance2);
		
		p[0] = std::cos(angle) * distance;
		p[1] = 0.f;
		p[2] = std::sin(angle) * distance;
		
		const float angle2 = angle + random(-.2f, +.2f) + M_PI;
		const float speed = lerp(.2f, 1.f, std::pow(random(0.f, 1.f), 2.f));
		
		v[0] = std::cos(angle2) * speed;
		v[1] = 0.f;
		v[2] = std::sin(angle2) * speed;
		
		//v[1] += 2.f;
		v[1] += lerp(1.f, 10.f, std::pow(random(0.f, 1.f), 2.f));
	}
	
	void updateTransform()
	{
		const float rX = p[0] + p[1];
		const float rY = p[2] + p[1];
		const float scale = .1f;
		
		audioSource.soundVolume.transform =
			Mat4x4(true)
			.Translate(p)
			.RotateY(rY)
			.RotateX(rX)
			.Scale(scale, scale, scale);
	}
	
	void tickAudio(const float dt)
	{
		rampUp = false;
		
		p += v * dt;
		
	#if 0
		if (p[1] < 0.f)
		{
			rampUp = true;
			rampUpVolume = std::abs(v[1] * .2f);
			
			p[1] = 0.f;
			v[1] *= -.9f;
		}
	#endif
		
	#if 1
		const float height = sampleHeight(p);
		
		if (p[1] < height)
		{
			const float delta = p[1] - height;
			
			rampUp = true;
			rampUpVolume = std::abs(delta * 1.f);
			
			v[1] -= delta * 200.f * dt;
			
			const float falloff = std::pow(.5f, dt);
			v[1] *= falloff;
		}
	#else
		const float height = sampleHeight(p);
		
		if (p[1] < height && v[1] < 0.f)
		{
			p[1] = height;
		}
	#endif
		
		v[1] -= 8.f * dt;
		
		const float distance = p.CalcSize();
		
		if (distance > kMaxSpeederDistance)
		{
			randomizePosition();
		}
		
		updateTransform();
	}
	
	void tick(const float dt)
	{
	}
	
	virtual void generate(SAMPLE_ALIGN16 float * __restrict samples, const int numSamples) override
	{
		monoData.generate(samples, numSamples);
		
		const float desiredGain = rampUp ? rampUpVolume : 0.1f;
		const float retain = std::pow(.1f, 1.f / AUDIO_UPDATE_SIZE);
		const float falloff = 1.f - retain;
		
		for (int i = 0; i < AUDIO_UPDATE_SIZE; ++i)
		{
			gain = gain * retain + desiredGain * falloff;
			
			samples[i] += random(-1.f, +1.f) * gain * .5f;
		}
	}
};

struct RecordedData
{
	float samples[NUM_BUFFERS_PER_RECORDING * AUDIO_UPDATE_SIZE];
	int numSamples;
};

struct Recorder
{
	RecordedData recordedData;
	
	Recorder()
		: recordedData()
	{
	}
	
	void tickAudio(const float dt)
	{
		memcpy(recordedData.samples + recordedData.numSamples, s_inputData, AUDIO_UPDATE_SIZE * sizeof(float));
		recordedData.numSamples += AUDIO_UPDATE_SIZE;
		
		if (recordedData.numSamples == NUM_BUFFERS_PER_RECORDING * AUDIO_UPDATE_SIZE)
		{
			addRecordedFragment(recordedData);
			
			recordedData.numSamples = 0;
		}
	}
};

struct RecordedFragment : AudioSource
{
	Vec3 p;
	Vec3 v;
	
	float gain;
	bool rampUp;
	float rampUpVolume;
	
	RecordedData recordedData;
	
	int playbackPosition;
	
	MultiChannelAudioSource_SoundVolume audioSource;
	
	RecordedFragment()
		: p()
		, v()
		, gain(0.f)
		, rampUp(false)
		, rampUpVolume(0.f)
		, recordedData()
		, playbackPosition(0)
		, audioSource()
	{
	}
	
	void init()
	{
		audioSource.init(this);
		
		randomizePosition();
		updateTransform();
		
		s_paHandler->addAudioSource(&audioSource);
	}
	
	void shut()
	{
		s_paHandler->removeAudioSource(&audioSource);
	}
	
	void randomizePosition()
	{
		const float angle = random(0.f, float(M_PI) * 2.f);
		const float distance = random(kInitSpeederDistance1, kInitSpeederDistance2);
		
		p[0] = std::cos(angle) * distance;
		p[1] = 0.f;
		p[2] = std::sin(angle) * distance;
		
		const float angle2 = angle + random(-.2f, +.2f) + M_PI;
		const float speed = lerp(.2f, 2.f, std::pow(random(0.f, 1.f), 2.f));
		
		v[0] = std::cos(angle2) * speed;
		v[1] = 0.f;
		v[2] = std::sin(angle2) * speed;
		
		v[1] += lerp(1.f, 10.f, std::pow(random(0.f, 1.f), 2.f));
	}
	
	void updateTransform()
	{
		const float rX = (p[0] + p[1]) * 4.f;
		const float rY = (p[2] + p[1]) * 4.f;
		const float scale = .1f;
		
		audioSource.soundVolume.transform =
			Mat4x4(true)
			.Translate(p)
			.RotateY(rY)
			.RotateX(rX)
			.Scale(scale, scale, scale);
	}
	
	void tickAudio(const float dt)
	{
		rampUp = false;
		
		p += v * dt;
		
		if (p[1] < 0.f)
		{
			rampUp = true;
			rampUpVolume = std::abs(v[1] * .5f);
			
			p[1] = 0.f;
			v[1] *= -.9f;
		}
		
		v[1] -= 8.f * dt;
		
		const float distance = p.CalcSize();
		
		if (distance > kMaxSpeederDistance)
		{
			randomizePosition();
		}
		
		updateTransform();
	}
	
	void tick(const float dt)
	{
	}
	
	virtual void generate(SAMPLE_ALIGN16 float * __restrict samples, const int numSamples) override
	{
		for (int i = 0; i < numSamples; ++i)
		{
			samples[i] = recordedData.samples[playbackPosition];
			
			playbackPosition++;
			
			if (playbackPosition == recordedData.numSamples)
				playbackPosition = 0;
		}
		
	#if 1
		const float desiredGain = rampUp ? rampUpVolume : 0.01f;
		const float retain = std::pow(.1f, 1.f / AUDIO_UPDATE_SIZE);
		const float falloff = 1.f - retain;
		
		for (int i = 0; i < AUDIO_UPDATE_SIZE; ++i)
		{
			gain = gain * retain + desiredGain * falloff;
			
			samples[i] += random(-1.f, +1.f) * gain;
		}
	#endif
	}
};

struct World
{
	Speeder speeders[NUM_SPEEDERS];
	
	std::deque<RecordedFragment*> recordedFragments;
	
	Recorder recorder;
	
#if DO_WATER
	Waves waves;
#endif
	
	void init()
	{
		for (auto & speeder : speeders)
		{
			speeder.init();
		}
	}
	
	void shut()
	{
		for (auto & recordedFragment : recordedFragments)
		{
			delete recordedFragment;
		}
		
		recordedFragments.clear();
	}
	
	void tickAudio(const float dt)
	{
		for (auto & speeder : speeders)
		{
			speeder.tickAudio(dt);
		}
		
		for (auto & recordedFragment : recordedFragments)
		{
			recordedFragment->tickAudio(dt);
		}
		
		recorder.tickAudio(dt);
		
	#if DO_WATER
		waves.tick(dt);
	#endif
	}
	
	void tick(const float dt)
	{
		for (auto & speeder : speeders)
		{
			speeder.tick(dt);
		}
	}
};

static World * s_world = nullptr;

static void tickAudio(const float dt)
{
	s_world->tickAudio(dt);
}

static void addRecordedFragment(RecordedData & recordedData)
{
#if DO_RECORDING
	if (s_world->recordedFragments.size() == MAX_SOUNDVOLUMES)
	{
		RecordedFragment * fragment = s_world->recordedFragments.front();
		
		fragment->shut();
		delete fragment;
		fragment = nullptr;
		
		s_world->recordedFragments.pop_front();
	}
	
	if (s_world->recordedFragments.size() < MAX_SOUNDVOLUMES)
	{
		RecordedFragment * fragment = new RecordedFragment();
		fragment->recordedData = recordedData;
		fragment->init();
		
		s_world->recordedFragments.push_back(fragment);
	}
#endif
}

static float sampleHeight(Vec3Arg p)
{
#if DO_WATER
	const Vec3 p_waves = s_world->waves.projectToWaves(p);
	
	const float height = s_world->waves.sample(p_waves[0], p_waves[1]);
	
	const Vec3 p_world = s_world->waves.transform * Vec3(p_waves[0], p_waves[1], height);
	
	return p_world[1];
#else
	return 0.f;
#endif
}

static void drawSoundVolume(const SoundVolume & volume)
{
	gxPushMatrix();
	{
		gxMultMatrixf(volume.transform.m_v);
		
	#if 1
		//const int res = 4;
		const int res = 1;
		
		gxPushMatrix(); { gxTranslatef(-1, 0, 0); drawGrid3dLine(res, res, 1, 2); } gxPopMatrix();
		gxPushMatrix(); { gxTranslatef(+1, 0, 0); drawGrid3dLine(res, res, 1, 2); } gxPopMatrix();
		gxPushMatrix(); { gxTranslatef(0, -1, 0); drawGrid3dLine(res, res, 2, 0); } gxPopMatrix();
		gxPushMatrix(); { gxTranslatef(0, +1, 0); drawGrid3dLine(res, res, 2, 0); } gxPopMatrix();
		gxPushMatrix(); { gxTranslatef(0, 0, -1); drawGrid3dLine(res, res, 0, 1); } gxPopMatrix();
		gxPushMatrix(); { gxTranslatef(0, 0, +1); drawGrid3dLine(res, res, 0, 1); } gxPopMatrix();
	#endif
	
		gxSetTexture(getTexture("thegrooop/logo-white.png"));
		{
			drawRect(-1, -1, +1, +1);
		}
		gxSetTexture(0);
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

#if DO_WATER

static void drawWaves_solid(const Waves & waves)
{
	gxPushMatrix();
	{
		gxMultMatrixf(waves.transform.m_v);
		
		setColor(100, 100, 100);
		gxBegin(GX_TRIANGLES);
		{
			for (int x = 0; x < waves.kSize - 1; ++x)
			{
				for (int y = 0; y < waves.kSize - 1; ++y)
				{
					const int x0 = x + 0;
					const int y0 = y + 0;
					const int x1 = x + 1;
					const int y1 = y + 1;
					const float z00 = waves.p[x0][y0];
					const float z10 = waves.p[x1][y0];
					const float z01 = waves.p[x0][y1];
					const float z11 = waves.p[x1][y1];
					
					gxColor4f(z00, z00, z00, 1.f);
					
					gxVertex3f(x0, y0, z00);
					gxVertex3f(x1, y0, z10);
					gxVertex3f(x1, y1, z11);
					
					gxVertex3f(x0, y0, z00);
					gxVertex3f(x1, y1, z11);
					gxVertex3f(x0, y1, z01);
				}
			}
		}
		gxEnd();
	}
	gxPopMatrix();
}

#endif

int main(int argc, char * argv[])
{
#if defined(CHIBI_RESOURCE_PATH)
	changeDirectory(CHIBI_RESOURCE_PATH);
#endif

	framework.enableDepthBuffer = true;
	//framework.fullscreen = true;
	
	if (!framework.init(GFX_SX, GFX_SY))
		return -1;
	
	Camera3d camera;
	camera.gamepadIndex = 0;
	
	camera.position[0] = 0;
	camera.position[1] = +.3f;
	camera.position[2] = -1.f;
	camera.pitch = 10.f;
	
	float fov = 70.f;
	float near = .05f;
	float far = 20.f;
	
	SDL_mutex * audioMutex = SDL_CreateMutex();
	
	MyMutex binauralMutex(audioMutex);
	s_binauralMutex = &binauralMutex;
	
	binaural::HRIRSampleSet sampleSet;
	binaural::loadHRIRSampleSet_Cipic("binaural/CIPIC/subject147", sampleSet);
	sampleSet.finalize();
	s_sampleSet = &sampleSet;
	
	MyPortAudioHandler * paHandler = new MyPortAudioHandler();
	s_paHandler = paHandler;
	
	World world;
	world.init();
	s_world = &world;
	
	PortAudioObject pa;
	pa.init(SAMPLE_RATE, 2, 1, AUDIO_UPDATE_SIZE, paHandler);
	
	do
	{
		framework.process();

		const float dt = framework.timeStep;
		
		// update the camera
		
		const bool doCameraInput = !(keyboard.isDown(SDLK_LSHIFT) || keyboard.isDown(SDLK_RSHIFT));
		
		camera.tick(dt, doCameraInput);
		
		// update the world
		
		world.tick(dt);
		
		// update information to/from audio thread
		
		SoundVolume soundVolumes[MAX_SOUNDVOLUMES];
		int numSoundVolumes = 0;
		
	#if DO_WATER
		Waves waves;
	#endif
		
		s_binauralMutex->lock();
		{
			paHandler->worldToViewTransform = camera.getViewMatrix();
			
			for (auto & audioSource : paHandler->audioSources)
			{
				soundVolumes[numSoundVolumes++] = audioSource->soundVolume;
			}
			
		#if DO_WATER
			waves = world.waves;
			
			for (int xo = -2; xo <= +2; ++xo)
			{
				 for (int yo = -2; yo <= +2; ++yo)
				 {
					const int x = xo + mouse.x * Waves::kSize / GFX_SX;
					const int y = yo + mouse.y * Waves::kSize / GFX_SY;
				
					if (x >= 0 && x < Waves::kSize &&
						y >= 0 && y < Waves::kSize)
					{
						const float value = (1.f - sqrt(xo * xo + yo * yo) / 3.f) * 4.f;
						const float retain = std::pow(.2f, dt);
						
						world.waves.p[x][y] = lerp(value, world.waves.p[x][y], retain);
					}
				}
			}
		#endif
		}
		s_binauralMutex->unlock();
		
		framework.beginDraw(0, 0, 0, 0, 1.f);
		{
			pushFontMode(FONT_SDF);
			setFont("calibri.ttf");
			
			projectPerspective3d(fov, near, far);
			
			camera.pushViewMatrix();
			{
				pushLineSmooth(true);
				
				pushDepthTest(true, DEPTH_LESS);
				{
				#if DO_WATER
					drawWaves_solid(waves);
				#endif
					
					for (int i = 0; i < numSoundVolumes; ++i)
					{
						setColor(160, 160, 160);
						drawSoundVolume(soundVolumes[i]);
					}
				}
				popDepthTest();
				
				//
				
				pushDepthTest(true, DEPTH_LESS, false);
				pushBlend(BLEND_ADD);
				{
					gxPushMatrix();
					{
						gxScalef(10, 10, 10);
						setColor(16, 20, 24);
						drawGrid3dLine(100, 100, 0, 2, true);
						
						setColor(60, 100, 160, 127);
						drawGrid3d(1, 1, 0, 2);
					}
					gxPopMatrix();
					
					for (int i = 0; i < numSoundVolumes; ++i)
					{
						setColor(60, 100, 160, 63);
						drawSoundVolume_Translucent(soundVolumes[i]);
					}
				}
				popBlend();
				popDepthTest();
				
				popLineSmooth();
			}
			camera.popViewMatrix();
			
			projectScreen2d();
			
		#if DO_WATER && 0
			const Vec3 coord = waves.projectToWaves(camera.position);
			const float value = waves.sample(coord[0], coord[1]);
			setColor(colorWhite);
			drawText(10, 10, 16, +1, +1, "wave value: %.2f", value);
		#endif
		
			popFontMode();
		}
		framework.endDraw();
	} while (!keyboard.wentDown(SDLK_ESCAPE));
	
	pa.shut();
	
	world.shut();
	s_world = nullptr;
	
	s_paHandler = nullptr;
	delete paHandler;
	paHandler = nullptr;
	
	s_sampleSet = nullptr;
	
	s_binauralMutex = nullptr;
	
	SDL_DestroyMutex(audioMutex);
	audioMutex = nullptr;
	
	framework.shutdown();
	
	return 0;
}
