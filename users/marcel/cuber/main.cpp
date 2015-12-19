#include "AudioFFT.h"
#include "Calc.h"
#include "config.h"
#include "cube.h"
#include "framework.h"
#include "GameOfLife.h"
#include "Path.h"
#include "script.h"
#include "simplexnoise.h"

#include "audiostream/AudioOutput.h"
#include "audiostream/AudioStreamVorbis.h"

#include <Windows.h>

//

static const int kFFTBufferSize = 4096;
static const int kFFTSize = 1024;
static const int kFFTComplexSize = 513; // n/2+1
static const int kFFTBucketCount = 32;
static audiofft::AudioFFT s_fft;

static float s_fftInputBuffer[4096];
static float s_fftInput[kFFTSize] = { };
static float s_fftReal[kFFTComplexSize] = { };
static float s_fftImaginary[kFFTComplexSize] = { };
static float s_fftBuckets[kFFTBucketCount] = { };

static float s_fftProvideTime = 0.f;

static void fftInit()
{
	s_fft.init(kFFTSize);
}

static float fftPowerValue(int i)
{
	float p = s_fftReal[i] * s_fftReal[i] + s_fftImaginary[i] * s_fftImaginary[i];
#if 1
	p = sqrtf(p);
#else
	p = 10.f * std::log10f(p);
#endif
	return p;
}

static void fftProcess()
{
	const float dt = framework.time - s_fftProvideTime;
	int sampleStart = dt * 44100.f; // fixme
	if (sampleStart + kFFTSize > kFFTBufferSize)
		sampleStart = kFFTBufferSize - kFFTSize;

	//sampleStart = 0;

	s_fft.fft(s_fftInputBuffer + sampleStart, s_fftReal, s_fftImaginary);

	for (int i = 0; i < kFFTBucketCount; ++i)
	{
		const int numSamples = kFFTComplexSize / kFFTBucketCount;
		const int j1 = (i + 0) * numSamples;
		const int j2 = (i + 1) * numSamples;
		Assert(j2 <= kFFTSize);

		float result = 0.f;

		for (int j = j1; j < j2; ++j)
			result += fftPowerValue(j);

		s_fftBuckets[i] = result / numSamples;
	}
}

//

// fixme : move these

float EffectCtxImpl::fftBucketValue(int index) const
{
	return s_fftBuckets[index];
}

int EffectCtxImpl::fftBucketCount() const
{
	return kFFTBucketCount;
}

//

#include "FileStream.h"
#include "FileStreamExtends.h"
#include "MemoryStream.h"

static const char * kDllCopyFilename = "script-copy.dll";

class ScriptEffect : public Effect
{
	std::string m_filename;

	HINSTANCE m_scriptInstance;

	CreateFunction m_createFunction;
	DestroyFunction m_destroyFunction;

	EffectCtx & m_effectCtx;
	Effect * m_effect;

	//

	HANDLE m_fileChanged;

public:
	ScriptEffect(EffectCtx & ctx, const char * filename)
		: m_scriptInstance(NULL)
		, m_createFunction(nullptr)
		, m_destroyFunction(nullptr)
		, m_effectCtx(ctx)
		, m_effect(nullptr)
	{
		init(filename);
	}

	~ScriptEffect()
	{
		shutdown();
	}

	//

	void init(const char * filename)
	{
		m_filename = filename;

		syncDll();

		m_scriptInstance = LoadLibraryA(kDllCopyFilename);

		if (m_scriptInstance != NULL)
		{
			m_createFunction = (CreateFunction)GetProcAddress(m_scriptInstance, "create");
			m_destroyFunction = (DestroyFunction)GetProcAddress(m_scriptInstance, "destroy");
		}

		if (m_createFunction)
			m_effect = m_createFunction(m_effectCtx);

		//

		const std::string path = Path::GetDirectory(m_filename);

		m_fileChanged = FindFirstChangeNotificationA(path.c_str(), FALSE, FILE_NOTIFY_CHANGE_LAST_WRITE);
	}

	void shutdown()
	{
		if (m_effect && m_destroyFunction)
		{
			m_destroyFunction(m_effect);
			m_effect = nullptr;
		}

		m_createFunction = nullptr;
		m_destroyFunction = nullptr;

		if (m_scriptInstance != NULL)
		{
			FreeLibrary(m_scriptInstance);
			m_scriptInstance = NULL;
		}
	}

	bool hasDllChanged() const
	{
		try
		{
			FileStream src;
			FileStream dst;
			src.Open(m_filename.c_str(), OpenMode_Read);
			dst.Open(kDllCopyFilename, OpenMode_Read);

			return !FileStreamExtents::ContentsAreEqual(&src, &dst);
		}
		catch (std::exception & e)
		{
			logError(e.what());
			return false;
		}
	}

	void syncDll() const
	{
		try
		{
			MemoryStream src;
			FileStream fileStream;
			fileStream.Open(m_filename.c_str(), OpenMode_Read);
			StreamExtensions::StreamTo(&fileStream, &src, 1024*1024);

			FileStreamExtents::OverwriteIfChanged(&src, kDllCopyFilename);
		}
		catch (std::exception & e)
		{
			logError(e.what());
		}
	}

	void reload()
	{
		logDebug("reloading script DLL");

		shutdown();

		init(m_filename.c_str());
	}

	//

	virtual void tick(const float dt)
	{
		if (WaitForSingleObject(m_fileChanged, 0) == WAIT_OBJECT_0)
		{
			if (hasDllChanged())
			{
				reload();
			}

			BOOL result = FindNextChangeNotification(m_fileChanged);
			Assert(result == TRUE);
		}

		if (m_effect)
		{
			m_effect->tick( dt);
		}
	}

	virtual void evaluateRaw(int x, int y, int z, float & value)
	{
		if (m_effect)
			m_effect->evaluateRaw(x, y, z, value);
	}

	virtual float evaluate(const Coord & coord)
	{
		if (m_effect)
			return m_effect->evaluate(coord);
		else
			return 0.f;
	}

	virtual void debugDraw()
	{
		if (m_effect)
			m_effect->debugDraw();
	}
};

//

static const float kParticleDirInterval = 1.f;
static const float kParticlePosInterval = .1f;

class MyEffect : public Effect
{
	const static int kNumParticles = 10;

	float m_time;
	Transform m_testMatrix;
	Transform m_pointMatrix1;
	Transform m_pointMatrix2;
	Transform m_particleMatrix;
	Particle m_particles[kNumParticles];

	enum Test
	{
		kTest_None,
		kTest_Calibrate,
		kTest_MinParticleDistance,
		kTest_CircleDistance,
		kTest_SphereDistance,
		kTest_CubeDistance,
		kTest_ToroidDistance,
		kTest_CircleHullDistance,
		kTest_PointDistance,
		kTest_LineDistance,
		kTest_LineSegmentDistance,
		kTest_PerlineNoise,
		kTest_Torus82,
		kTest_Torus88,
		kTest_COUNT
	};

	enum TransformMode
	{
		kTransformMode_Identity,
		kTransformMode_Scale,
		kTransformMode_Rotate,
		kTransformMode_Translate,
		kTransformMode_COUNT
	};

	Test m_test;
	TransformMode m_transform;

	int m_bucketIndex;

	GameOfLife<SX, SY, 1> m_gameOfLife;
	float m_gameOfLifeUpdateTimer;

	CubeBuffer m_particleBuffer;
	struct ShootingStar
	{
		enum Randomize
		{
			kRandomize_Position = 1 << 0,
			kRandomize_Direction = 1 << 1
		};

		enum CollisionResponse
		{
			kCollisionResponse_Wrap,
			kCollisionResponse_Bounce,
			kCollisionResponse_Respawn
		};

		int m_randomize;
		CollisionResponse m_collisionResponse;

		int m_particleDir[3];
		float m_particleDirTimer;
		int m_particlePos[3];
		float m_particlePosTimer;

		ShootingStar()
			: m_randomize((kRandomize_Direction * 1) | (kRandomize_Position * 0))
			, m_collisionResponse(kCollisionResponse_Wrap)
			//, m_collisionResponse(kCollisionResponse_Respawn)
			//, m_collisionResponse(kCollisionResponse_Bounce)
			, m_particleDirTimer(kParticleDirInterval)
			, m_particlePosTimer(kParticlePosInterval)
		{
			randomizePosition();
			randomizeDirection();
		}

		void getNextPosition(int * xyz) const
		{
			for (int i = 0; i < 3; ++i)
				xyz[i] = m_particlePos[i] + m_particleDir[i];
		}

		void randomizePosition()
		{
			m_particlePos[0] = rand() % SX;
			m_particlePos[1] = rand() % SY;
			m_particlePos[2] = rand() % SZ;
		}

		void randomizeDirection()
		{
			m_particleDir[0] = 0;
			m_particleDir[1] = 0;
			m_particleDir[2] = 0;
			m_particleDir[rand() % 3] = (rand() % 2) ? -1 : +1;
		}

		void tick(float dt, CubeBuffer & buffer)
		{
			m_particleDirTimer += dt;
			while (m_particleDirTimer >= kParticleDirInterval)
			{
				m_particleDirTimer -= kParticleDirInterval;
				if (m_randomize & kRandomize_Position)
					randomizePosition();
				if (m_randomize & kRandomize_Direction)
					randomizeDirection();
			}

			m_particlePosTimer += dt;
			while (m_particlePosTimer >= kParticlePosInterval)
			{
				m_particlePosTimer -= kParticlePosInterval;
				buffer.m_value[m_particlePos[0]][m_particlePos[1]][m_particlePos[2]] = 1.f;

				const int wrapSizes[3] = { SX, SY, SZ };

				for (int i = 0; i < 3; ++i)
				{
					if (m_collisionResponse == kCollisionResponse_Wrap)
						m_particlePos[i] = (m_particlePos[i] + m_particleDir[i] + wrapSizes[i]) % wrapSizes[i];
					else if (m_collisionResponse == kCollisionResponse_Bounce)
					{
						int newPos = m_particlePos[i] + m_particleDir[i];
						if (newPos < 0 || newPos >= wrapSizes[i])
						{
							m_particleDir[i] *= -1;
							newPos = m_particlePos[i] + m_particleDir[i];
						}
						m_particlePos[i] = newPos;
					}
					else if (m_collisionResponse == kCollisionResponse_Respawn)
					{
						const int newPos = m_particlePos[i] + m_particleDir[i];
						if (newPos < 0 || newPos >= wrapSizes[i])
						{
							randomizePosition();
							randomizeDirection();
							break;
						}
						else
							m_particlePos[i] = newPos;
					}
				}
			}
		}
	};

	static const int kNumShootingStars = 10;
	ShootingStar m_shootingStars[kNumShootingStars];

public:
	MyEffect(EffectCtx & ctx)
		: m_time(0.f)
		, m_test(kTest_None)
		, m_transform(kTransformMode_Identity)
		, m_bucketIndex(0)
		, m_gameOfLifeUpdateTimer(0.f)
	{
		for (int i = 0; i < kNumParticles; ++i)
		{
			m_particles[i].active = true;
			m_particles[i].x = rand(-1.f, +1.f);
			m_particles[i].y = rand(-1.f, +1.f);
			m_particles[i].z = rand(-1.f, +1.f);
		}

		m_gameOfLife.randomize();
	}

	virtual void tick(EffectCtx & ctx, const float dt)
	{
		if (keyboard.wentDown(SDLK_LEFT))
			m_bucketIndex = Calc::Max(0, m_bucketIndex - 1);
		if (keyboard.wentDown(SDLK_RIGHT))
			m_bucketIndex = Calc::Min(m_bucketIndex + 1, kFFTBucketCount - 1);

		if (keyboard.wentDown(SDLK_t))
			m_test = (Test)((m_test + (keyboard.isDown(SDLK_LSHIFT) ? -1 : 1) + kTest_COUNT) % kTest_COUNT);
		if (keyboard.wentDown(SDLK_x))
			m_transform = (TransformMode)((m_transform + (keyboard.isDown(SDLK_LSHIFT) ? -1 : 1) + kTransformMode_COUNT) % kTransformMode_COUNT);

		m_time += dt;

		//

		m_testMatrix.reset();
		m_testMatrix.scale(.5f, .5f, .5f);

		switch (m_transform)
		{
		case kTransformMode_Identity:
			break;

		case kTransformMode_Scale:
			{
				const float s = std::sinf(m_time);

				m_testMatrix.scale(s, s, s);
			}
			break;

		case kTransformMode_Rotate:
			{
				m_testMatrix.rotateX(m_time / 1.111f);
				m_testMatrix.rotateY(m_time / 1.333f);
				m_testMatrix.rotateZ(m_time / 1.777f);
			}
			break;

		case kTransformMode_Translate:
			{
				const float dx = std::sinf(m_time / 1.111f);
				const float dy = std::sinf(m_time / 1.333f);
				const float dz = std::sinf(m_time / 1.777f);

				m_testMatrix.translate(dx, dy, dz);
			}
			break;
		}

		//

		m_pointMatrix1.reset();
		m_pointMatrix1.scale(.2f, .2f, .2f);
		m_pointMatrix1.rotateY(m_time);
		m_pointMatrix1.rotateZ(m_time);

		m_pointMatrix2.reset();
		m_pointMatrix2.scale(.2f, .2f, .2f);
		m_pointMatrix2.rotateZ(m_time);
		m_pointMatrix2.rotateY(m_time);

		m_particleMatrix.reset();
		m_particleMatrix.rotateX(m_time);

		//

		const float kGameOfLifeUpdateInterval = .2f;
		m_gameOfLifeUpdateTimer += dt;
		while (m_gameOfLifeUpdateTimer >= kGameOfLifeUpdateInterval)
		{
			m_gameOfLifeUpdateTimer -= kGameOfLifeUpdateInterval;
			m_gameOfLife.evolve();
		}

		//

		m_particleBuffer.fadeLinear(.25f, dt);

		for (int i = 0; i < kNumShootingStars; ++i)
			m_shootingStars[i].tick(dt, m_particleBuffer);
	}

	virtual void evaluateRaw(int x, int y, int z, float & value)
	{
		value = max(value, m_particleBuffer.m_value[x][y][z]);

	#if 0
		//if (z == SZ / 2)
			if (m_gameOfLife.sample(x, y, z))
				value = max(value, 1.f);
	#endif
	}

	virtual float evaluate(const Coord & c)
	{
		const float controlX = mouse.x / float(800.f);
		const float controlY = mouse.y / float(800.f);

		const float power = keyboard.isDown(SDLK_p) ? 1.f : s_fftBuckets[m_bucketIndex];

		bool doTestPostEffects = true;

		const Coord testCoord = m_testMatrix.apply(c);

		float d;

		switch (m_test)
		{
		case kTest_None:
			{
				doTestPostEffects = false;

			#if 0
				const float s = 10.f;
				const float t1 = (sinf(m_time * s / 1.111f) + 1.f) * .25f;
				const float t2 = (sinf(m_time * s / 2.333f) + 1.f) * .25f;
				d = computePlaneDistance(twistZ(twistY(c, t1), t2)) * 2.f;
				d = (1.f - pow(d, power));
			#elif 0
				const float r = .4f + .3f * sinf(m_time);
				d = computeCircleHullDistance(repeat(c, 0.f, 0.f, r)) * 3.f;
				d = 1.f - pow(d, power);
			#elif 0
				const Coord c1 = c;
				const Coord c2 = c + Coord(5.2f, 1.3f, 0.f);
				const Coord c3(
					computePerlinNoise(c1, m_time / 1.111f),
					computePerlinNoise(c2, m_time / 1.333f),
					0.f);

				d = computePerlinNoise(c + c3, m_time / 5.777f) * 3.f;
			#elif 0
				d = computePerlinNoise(c, m_time) * 3.f;
			#else
				auto c1 = m_pointMatrix1.apply(c);
				auto c2 = m_pointMatrix2.apply(c);
				//const float d = 1.f - computePointDistance(x, y, z);
				//const float d = 1.f - pow(computeLineDistance(x, y, z), 4.f);
				const float d1 = computePlaneDistance(twistY(c1, .05f));
				const float d2 = computePlaneDistance(twistZ(c2, .03f));
				const float d3 = computePerlinNoise(testCoord, m_time) * 2.f + 1.f;
				const float d4 = computeMinParticleDistance(m_particleMatrix.apply(c), m_particles, kNumParticles) * 4.f;
				d = min(d1, d2, d3, d4);
				//d = min(d4);
				d = (1.f - pow(d, power));
			#endif
			}
			break;

		case kTest_Calibrate:
			{
				doTestPostEffects = false;

				Transform topMatrix;
				topMatrix.translate(0.f, -1.f, 0.f);
				topMatrix.rotateZ(M_PI/2.f);
				d = computePlaneDistance(topMatrix.apply(c));

				if (keyboard.isDown(SDLK_LSHIFT))
				{
					Transform leftMatrix;
					leftMatrix.translate(-1.f, 0.f, 0.f);
					//d = csgUnion(d, computePlaneDistance(leftMatrix.apply(c)));
					//d = csgSoftUnion(d, computePlaneDistance(leftMatrix.apply(c)), controlY * 4.f);
					d = csgSoftIntersection(d, computePlaneDistance(leftMatrix.apply(c)), controlY * 4.f);
				}

				d = 1.f - d * 4.f;
			}
			break;

		case kTest_MinParticleDistance:
			d = computeMinParticleDistance(testCoord, m_particles, kNumParticles) * 2.f;
			break;

		case kTest_CircleDistance:
			d = computeCircleDistance(testCoord);
			break;

		case kTest_SphereDistance:
			d = computeSphereDistance(testCoord);
			break;

		case kTest_CubeDistance:
			d = computeCubeDistance(testCoord, 1.f, 1.f, 1.f);
			break;

		case kTest_ToroidDistance:
			d = computeToroidDistance(testCoord, .2f, .8f);
			break;

		case kTest_CircleHullDistance:
			d = computeCircleHullDistance(testCoord);
			break;

		case kTest_PointDistance:
			d = computePointDistance(testCoord);
			break;

		case kTest_LineDistance:
			d = computeLineDistance(testCoord);
			break;

		case kTest_LineSegmentDistance:
			d = computeLineSegmentDistance(testCoord, -1.f, 0.f, .25f);
			break;

		case kTest_PerlineNoise:
			d = computePerlinNoise(testCoord, m_time) + 1.f;
			break;

		case kTest_Torus82:
			d = computeTorus82(testCoord, 1.f, .25f);
			break;

		case kTest_Torus88:
			d = computeTorus88(testCoord, 1.f, .25f);
			break;
		}

		if (d < 0.f)
			d = 0.f;

		if (doTestPostEffects)
			d = 1.f - pow(d, power);

		return d;
	}

	void debugDraw()
	{
		setColor(colorWhite);
		drawText(mouse.x, mouse.y + 25, 12, 0, 1, "power:%.2f", s_fftBuckets[m_bucketIndex]);
	}
};

static void evalCube(Cube & cube, Effect * effect)
{
	for (int x = 0; x < SX; ++x)
	{
		for (int y = 0; y < SY; ++y)
		{
			for (int z = 0; z < SZ; ++z)
			{
				Coord c;

				c.x = (x / (SX - 1.f) - .5f) * 2.f;
				c.y = (y / (SY - 1.f) - .5f) * 2.f;
				c.z = (z / (SZ - 1.f) - .5f) * 2.f;

				cube.m_value[x][y][z] = effect->evaluate(c);

				effect->evaluateRaw(x, y, z, cube.m_value[x][y][z]);
			}
		}
	}
}

static void drawCube(const Cube & cube)
{
	gxPushMatrix();
	{
		gxScalef(
			1.f / SX,
			1.f / SY,
			1.f / SZ);

		gxTranslatef(
			-(SX - 1) / 2.f,
			-(SY - 1) / 2.f,
			-(SZ - 1) / 2.f);

	#if 1
		gxBegin(GL_LINES);
		{
			gxColor4f(1.f, 1.f, 1.f, .25f);
			for (int x1 = 0; x1 <= 1; ++x1)
				for (int y1 = 0; y1 <= 1; ++y1)
					for (int z1 = 0; z1 <= 1; ++z1)
						for (int x2 = 0; x2 <= 1; ++x2)
							for (int y2 = 0; y2 <= 1; ++y2)
								for (int z2 = 0; z2 <= 1; ++z2)
									if (std::abs(x1-x2) + std::abs(y1-y2) + std::abs(z2-z1) == 1)
									{
										gxVertex3f(x1 * SX, y1 * SY, z1 * SZ);
										gxVertex3f(x2 * SX, y2 * SY, z2 * SZ);
									}
		}
		gxEnd();
	#endif

	#if VIDEO_RECORDING_MODE
		glPointSize(2.f);
	#endif
		setBlend(BLEND_ADD);

		gxBegin(GL_POINTS);
		{
			for (int x = 0; x < SX; ++x)
			{
				for (int y = 0; y < SY; ++y)
				{
					for (int z = 0; z < SZ; ++z)
					{
						const float value = cube.m_value[x][y][z];

						gxColor4f(value, value, value, 1.f);
						gxVertex3f(x, y, z);
					}
				}
			}
		}
		gxEnd();

		setBlend(BLEND_ALPHA);
	}
	gxPopMatrix();
}

class AudioStream_Capture : public AudioStream
{
public:
	AudioStream_Capture()
		: mSource(0)
	{
	}

	virtual int Provide(int numSamples, AudioSample* __restrict buffer)
	{
		if (mSource)
		{
			const int result = mSource->Provide(numSamples, buffer);
			const int copySize = Calc::Min(result, kFFTBufferSize);
			const float scale = 2.f / 65536.f;

			for (int i = 0; i < copySize; ++i)
				s_fftInputBuffer[i] = buffer[i].channel[0] * scale;
			for (int i = copySize; i < kFFTBufferSize; ++i)
				s_fftInputBuffer[i] = 0.f;

#if 0
			for (int i = 0; i < kFFTBufferSize / kFFTSize; ++i)
			{
				s_fft.fft(s_fftInputBuffer + kFFTSize * i, s_fftReal, s_fftImaginary);
				const int kMoveSize = 4;
				memmove(s_fftReal + kMoveSize, s_fftReal, sizeof(s_fftReal) - sizeof(float) * kMoveSize);
				memmove(s_fftImaginary + kMoveSize, s_fftImaginary, sizeof(s_fftImaginary) - sizeof(float) * kMoveSize);
				for (int j = 0; j < kMoveSize; ++j)
				{
					s_fftReal[j] = 0.f;
					s_fftImaginary[j] = 0.f;
				}
				s_fft.ifft(s_fftInputBuffer + kFFTSize * i, s_fftReal, s_fftImaginary);

				for (int j = 0; j < copySize; ++j)
					buffer[j].channel[0] = buffer[j].channel[1] = s_fftInputBuffer[j + kFFTSize * i] / scale;
			}
#endif

			s_fftProvideTime = framework.time;

			return result;
		}
		else
		{
			return 0;
		}
	}

	AudioStream * mSource;
};

class AudioIn
{
	HWAVEIN m_waveIn;
	WAVEFORMATEX m_waveFormat;
	WAVEHDR m_waveHeader;
	short * m_buffer;

public:
	AudioIn()
		: m_waveIn(nullptr)
		, m_buffer(nullptr)
	{
		memset(&m_waveFormat, 0, sizeof(m_waveFormat));
		memset(&m_waveHeader, 0, sizeof(m_waveHeader));
	}

	~AudioIn()
	{
		shutdown();
	}

	bool init(int channelCount, int sampleRate, int bufferSampleCount)
	{
		Assert((bufferSampleCount % channelCount) == 0);
		bufferSampleCount /= channelCount;

		//const int numDevices = waveInGetNumDevs();

		MMRESULT mmResult = MMSYSERR_NOERROR;

		memset(&m_waveFormat, 0, sizeof(m_waveFormat));
		m_waveFormat.wFormatTag = WAVE_FORMAT_PCM;
		m_waveFormat.nSamplesPerSec = sampleRate;
		m_waveFormat.nChannels = channelCount;
		m_waveFormat.wBitsPerSample = 16;
		m_waveFormat.nBlockAlign = m_waveFormat.nChannels * m_waveFormat.wBitsPerSample / 8;
		m_waveFormat.nAvgBytesPerSec = m_waveFormat.nSamplesPerSec * m_waveFormat.nBlockAlign;
		m_waveFormat.cbSize = 0;

		mmResult = waveInOpen(&m_waveIn, WAVE_MAPPER, &m_waveFormat, 0, 0, CALLBACK_NULL);
		Assert(mmResult == MMSYSERR_NOERROR);

		if (mmResult == MMSYSERR_NOERROR)
		{
			m_buffer = new short[bufferSampleCount * channelCount];

			memset(&m_waveHeader, 0, sizeof(m_waveHeader));
			m_waveHeader.lpData = (LPSTR)m_buffer;
			m_waveHeader.dwBufferLength = m_waveFormat.nBlockAlign * bufferSampleCount;
			m_waveHeader.dwBytesRecorded = 0;
			m_waveHeader.dwUser = 0;
			m_waveHeader.dwFlags = 0;
			m_waveHeader.dwLoops = 0;
			mmResult = waveInPrepareHeader(m_waveIn, &m_waveHeader, sizeof(m_waveHeader));
			Assert(mmResult == MMSYSERR_NOERROR);

			if (mmResult == MMSYSERR_NOERROR)
			{
				mmResult = waveInAddBuffer(m_waveIn, &m_waveHeader, sizeof(m_waveHeader));
				Assert(mmResult == MMSYSERR_NOERROR);

				if (mmResult == MMSYSERR_NOERROR)
				{
					mmResult = waveInStart(m_waveIn);
					Assert(mmResult == MMSYSERR_NOERROR);

					if (mmResult == MMSYSERR_NOERROR)
					{
						return true;
					}
				}
			}
		}

		shutdown();

		return false;
	}

	void shutdown()
	{
		MMRESULT mmResult = MMSYSERR_NOERROR;

		if (m_waveHeader.dwFlags & WHDR_PREPARED)
		{
			mmResult = waveInReset(m_waveIn);
			Assert(mmResult == MMSYSERR_NOERROR);

			mmResult = waveInUnprepareHeader(m_waveIn, &m_waveHeader, sizeof(m_waveHeader));
			Assert(mmResult == MMSYSERR_NOERROR);
		}

		if (m_waveIn)
		{
			mmResult = waveInClose(m_waveIn);
			Assert(mmResult == MMSYSERR_NOERROR);

			m_waveIn = nullptr;
		}

		delete [] m_buffer;
		m_buffer = nullptr;
	}

	bool provide(short * buffer, int & sampleCount)
	{
		MMRESULT mmResult = MMSYSERR_NOERROR;

	#if 0
		printf("wave header flags: %08x (prepared:%d, inqueue:%d, done:%d)\n",
			m_waveHeader.dwFlags,
			(m_waveHeader.dwFlags & WHDR_PREPARED) ? 1 : 0,
			(m_waveHeader.dwFlags & WHDR_INQUEUE) ? 1 : 0,
			(m_waveHeader.dwFlags & WHDR_DONE) ? 1 : 0);
	#endif

		if (m_waveHeader.dwFlags & WHDR_DONE)
		{
			memcpy(buffer, m_buffer, m_waveHeader.dwBytesRecorded);
			sampleCount = m_waveHeader.dwBytesRecorded / sizeof(short);

			mmResult = waveInAddBuffer(m_waveIn, &m_waveHeader, sizeof(m_waveHeader));
			Assert(mmResult == MMSYSERR_NOERROR);

			return true;
		}
		else
		{
			return false;
		}
	}
};

int main(int argc, char * argv[])
{
#if USE_AUDIO_INPUT
	AudioIn audioIn;

	audioIn.init(2, 44100, 4096 * 2);
#endif

#if 0
	for (;;)
	{
		short buffer[1024];
		int sampleCount = 0;

		if (audioIn.provide(buffer, sampleCount))
		{
			int sum = 0;
			for (int i = 0; i < sampleCount; ++i)
				sum += Calc::Abs(buffer[i]);
			printf("wave volume: %d\n", sum / sampleCount);
		}

		Sleep(10);
	}
#endif

	if (!framework.init(0, 0, 800, 800))
	{
		showErrorMessage("Startup Error", "Failed to initialise framework.");
	}
	else
	{
		fftInit();

	#if !USE_AUDIO_INPUT
		AudioStream_Vorbis audioStreamOGG;
		audioStreamOGG.Open("music1.ogg", true);

		AudioStream_Capture audioStream;
		audioStream.mSource = &audioStreamOGG;

		AudioOutput_OpenAL audioOutput;
		audioOutput.Initialize(2, audioStreamOGG.mSampleRate, 1 << 14);
		audioOutput.Play();
	#endif

		Cube cube;

		EffectCtxImpl ctx;
		//MyEffect effect(ctx);
	#ifdef DEBUG
		ScriptEffect effect(ctx, "Debug/script.dll");
	#else
		ScriptEffect effect(ctx, "Release/script.dll");
	#endif

		float rotation[3] = { };

		bool stop = false;

		while (!stop)
		{
			framework.process();

			// process input

			if (mouse.isDown(BUTTON_LEFT))
			{
				rotation[0] += mouse.dy / 100.f;
				rotation[1] -= mouse.dx / 100.f;
			}

			// process audio

		#if USE_AUDIO_INPUT
			short buffer[4096 * 2];
			int sampleCount = 0;
			if (audioIn.provide(buffer, sampleCount))
			{
				const float scale = 2.f / 65536.f;
				for (int i = 0; i < 4096; ++i)
					s_fftInputBuffer[i] = buffer[i * 2] * scale;
				s_fftProvideTime = framework.time;
			}
		#else
			audioOutput.Update(&audioStream);
		#endif

			// generate FFT

			fftProcess();

			// evaluate cube

			effect.tick(framework.timeStep);

			evalCube(cube, &effect);

			// todo : send output towards hardware

			framework.beginDraw(0, 0, 0, 0);
			{
				// draw debug visualisation

				setFont("calibri.ttf");

				gxMatrixMode(GL_PROJECTION);
				gxPushMatrix();
				{
					Mat4x4 t;
					t.MakePerspectiveGL(M_PI/2.f, 1.f, .1f, 10.f);
					gxLoadMatrixf(t.m_v);
					glScalef(1.f, -1.f, 1.f);

					gxMatrixMode(GL_MODELVIEW);
					gxPushMatrix();
					{
						const float scale = 1.f;

						gxTranslatef(0.f, 0.f, 1.5f);
						gxRotatef(rotation[0]*180.f/M_PI, 1.f, 0.f, 0.f);
						gxRotatef(rotation[1]*180.f/M_PI, 0.f, 1.f, 0.f);
						gxScalef(scale, scale, scale);
						drawCube(cube);
					}
					gxPopMatrix();
				}
				gxMatrixMode(GL_PROJECTION);
				gxPopMatrix();

				gxMatrixMode(GL_MODELVIEW);

				gxPushMatrix();
				{
					gxTranslatef(0.f, 750.f, 0.f);
					gxScalef(800.f, -10.f, 1.f);
					//gxScalef(2400.f, -10.f, 1.f);
					setColor(colorWhite);
					gxBegin(GL_LINE_LOOP);
					{
						gxVertex2f(0.f, 400.f);

						if (mouse.isDown(BUTTON_RIGHT))
						{
							for (int i = 0; i < kFFTBucketCount; ++i)
							{
								const float p = s_fftBuckets[i];
								const float x = i / float(kFFTBucketCount - 1);
								const float y = p;
								gxVertex2f(x, y);
							}
						}
						else
						{
							for (int i = 0; i < kFFTComplexSize; ++i)
							{
								const float p = fftPowerValue(i);
								const float x = i / float(kFFTComplexSize - 1);
								const float y = p;
								gxVertex2f(x, y);
							}
						}

						gxVertex2f(800.f, 400.f);
					}
					gxEnd();
				}
				gxPopMatrix();

				effect.debugDraw();
			}
			framework.endDraw();
		}

		framework.shutdown();
	}

	return 0;
}
