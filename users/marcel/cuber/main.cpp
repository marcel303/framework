#include "AudioFFT.h"
#include "Calc.h"
#include "framework.h"
#include "simplexnoise.h"

#include "audiostream/AudioOutput.h"
#include "audiostream/AudioStreamVorbis.h"

#define ArraySize(x) (sizeof(x) / sizeof(x[0]))

#define SX 20
#define SY 20
#define SZ 20

static float min(float x) { return x; }
static float min(float x, float y) { return x < y ? x : y; }
static float min(float x, float y, float z) { return min(x, min(y, z)); }
static float min(float x, float y, float z, float w) { return min(x, min(y, min(z, w))); }
static float min(float x, float y, float z, float w, float s) { return min(x, min(y, min(z, min(w, s)))); }

static float max(float x, float y)
{
	return x > y ? x : y;
}

static float max(float x, float y, float z)
{
	return max(x, max(y, z));
}

static float rand(float min, float max)
{
	return min + (max - min) * (rand() % 1024) / 1023.f;
}

struct Coord
{
	float x, y, z;
};

class Effect
{
public:
	virtual float evaluate(const Coord & c) = 0;
};

struct Cube
{
	Cube()
	{
		memset(this, 0, sizeof(*this));
	}

	float m_value[SX][SY][SZ];
};

class BiMatrix
{
	Mat4x4 m_matrix;
	Mat4x4 m_matrixInverse;
	bool m_dirty;

public:
	BiMatrix()
		: m_dirty(false)
	{
		m_matrix.MakeIdentity();
		m_matrixInverse.MakeIdentity();
	}

	void reset()
	{
		matrix().MakeIdentity();
	}

	void scale(float x, float y, float z)
	{
		Mat4x4 tempMatrix;
		tempMatrix.MakeScaling(x, y, z);
		matrix() = matrix() * tempMatrix;
	}

	void translate(float x, float y, float z)
	{
		Mat4x4 tempMatrix;
		tempMatrix.MakeTranslation(x, y, z);
		matrix() = matrix() * tempMatrix;
	}

	void rotateX(float r)
	{
		Mat4x4 tempMatrix;
		tempMatrix.MakeRotationX(r);
		matrix() = matrix() * tempMatrix;
	}

	void rotateY(float r)
	{
		Mat4x4 tempMatrix;
		tempMatrix.MakeRotationY(r);
		matrix() = matrix() * tempMatrix;
	}

	void rotateZ(float r)
	{
		Mat4x4 tempMatrix;
		tempMatrix.MakeRotationZ(r);
		matrix() = matrix() * tempMatrix;
	}

	Coord apply(const Coord & c)
	{
		const Vec3 v = matrixInverse().Mul4(Vec3(c.x, c.y, c.z));
		Coord r;
		r.x = v[0];
		r.y = v[1];
		r.z = v[2];
		return r;
	}

	Mat4x4 & matrix()
	{
		m_dirty = true;
		return m_matrix;
	}

	const Mat4x4 & matrixInverse()
	{
		if (m_dirty)
		{
			m_dirty = false;
			m_matrixInverse = m_matrix.CalcInv();
		}

		return m_matrixInverse;
	}
};

struct Triangle
{
};

struct Particle
{
	bool active;
	float x, y, z;

	Particle()
	{
		memset(this, 0, sizeof(*this));
	}
};

static float computeMinParticleDistance(const Coord & c, const Particle * particles, int numParticles);
static float computeTriangleDistance(const Coord & c);
static float computeCircleDistance(const Coord & c);
static float computePointDistance(const Coord & c);
static float computeLineDistance(const Coord & c);
static float computeLineSegmentDistance(const Coord & c);
static float computePerlinNoise(const Coord & c, float w);

static float length3(float x, float y, float z)
{
	return sqrtf(x * x + y * y + z * z);
}

static float computeMinParticleDistance(const Coord & c, const Particle * particles, int numParticles)
{
	float minDistance = std::numeric_limits<float>::max();

	for (int i = 0; i < numParticles; ++i)
	{
		if (!particles[i].active)
			return false;

		const float dx = c.x - particles[i].x;
		const float dy = c.y - particles[i].y;
		const float dz = c.z - particles[i].z;
		const float d = length3(dx, dy, dz);

		if (d < minDistance)
			minDistance = d;
	}

	return minDistance;
}

static float computeTriangleDistance(const Coord & c)
{
	return 0.f;
}

static float computeCircleDistance(const Coord & c)
{
	const float dx = c.x;
	const float dy = c.y;
	const float dz = 0.f;
	const float d = length3(dx, dy, dz);
	return d;
}

static float computePointDistance(const Coord & c)
{
	const float dx = c.x;
	const float dy = c.y;
	const float dz = c.z;
	const float d = length3(dx, dy, dz);
	return d;
}

static float computeLineDistance(const Coord & c)
{
	const float dx = 0.f;
	const float dy = c.y;
	const float dz = c.z;
	const float d = length3(dx, dy, dz);
	return d;
}

static float computePlaneDistance(const Coord & c)
{
	return std::abs(c.x);
}

static float computeLineSegmentDistance(const Coord & c)
{
	return 0.f;
}

static float computePerlinNoise(const Coord & c, float w)
{
	const float value = octave_noise_4d(8.f, .5f, .5f, c.x, c.y, c.z, w);

	return value;
}

class MyEffect : public Effect
{
	const static int kNumParticles = 10;

	float m_time;
	BiMatrix m_pointMatrix1;
	BiMatrix m_pointMatrix2;
	BiMatrix m_particleMatrix;
	Particle m_particles[kNumParticles];

public:
	MyEffect()
		: m_time(0.f)
	{
		for (int i = 0; i < kNumParticles; ++i)
		{
			m_particles[i].active = true;
			m_particles[i].x = rand(-1.f, +1.f);
			m_particles[i].y = rand(-1.f, +1.f);
			m_particles[i].z = rand(-1.f, +1.f);
		}
	}

	void tick(float dt)
	{
		m_time += dt;

		m_pointMatrix1.reset();
		m_pointMatrix1.scale(.2f, .2f, .2f);
		m_pointMatrix1.rotateY(m_time);
		m_pointMatrix1.rotateZ(m_time);
		//m_pointMatrix1.translate(m_time * .1f, 0.f, 0.f);

		m_pointMatrix2.reset();
		m_pointMatrix2.scale(.2f, .2f, .2f);
		m_pointMatrix2.rotateZ(m_time);
		m_pointMatrix2.rotateY(m_time);
		//m_pointMatrix2.translate(m_time * .1f, 0.f, 0.f);

		m_particleMatrix.reset();
		m_particleMatrix.rotateX(m_time);
	}

	virtual float evaluate(const Coord & c)
	{
#if 0
		const float d = computePerlinNoise(x, y, z, framework.time) * 3.f;
#else
		auto c1 = m_pointMatrix1.apply(c);
		auto c2 = m_pointMatrix2.apply(c);
		//const float d = 1.f - computePointDistance(x, y, z);
		//const float d = 1.f - pow(computeLineDistance(x, y, z), 4.f);
		const float d1 = computePlaneDistance(c1);
		const float d2 = computePlaneDistance(c2);
		const float d3 = computePerlinNoise(c, framework.time) + 1.f;
		const float d4 = computeMinParticleDistance(m_particleMatrix.apply(c), m_particles, kNumParticles) * 4.f;
		const float d = min(d1, d2, d3, d4);
		//const float d = min(d4);
		return 1.f - pow(d, 4.f);
#endif

		return d;
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
	}
	gxPopMatrix();
}

static const int kFFTBufferSize = 4096;
static const int kFFTSize = 1024;
static const int kFFTComplexSize = 513; // n/2+1
static audiofft::AudioFFT s_fft;

float s_fftInputBuffer[4096];
float s_fftInput[kFFTSize] = { };
float s_fftReal[kFFTComplexSize] = { };
float s_fftImaginary[kFFTComplexSize] = { };

static float s_fftProvideTime = 0.f;

static void fftInit()
{
	s_fft.init(kFFTSize);
}

static void fftProcess()
{
	const float dt = framework.time - s_fftProvideTime;
	int sampleStart = dt * 44100.f; // fixme
	if (sampleStart + kFFTSize > kFFTBufferSize)
		sampleStart = kFFTBufferSize - kFFTSize;

	//sampleStart = 0;

	s_fft.fft(s_fftInputBuffer + sampleStart, s_fftReal, s_fftImaginary);
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

int main(int argc, char * argv[])
{
	if (!framework.init(0, 0, 800, 800))
	{
		showErrorMessage("Startup Error", "Failed to initialise framework.");
	}
	else
	{
		fftInit();

		AudioStream_Vorbis audioStreamOGG;
		audioStreamOGG.Open("music.ogg", true);

		AudioStream_Capture audioStream;
		audioStream.mSource = &audioStreamOGG;

		AudioOutput_OpenAL audioOutput;
		audioOutput.Initialize(2, audioStreamOGG.mSampleRate, 1 << 14);
		audioOutput.Play();

		Cube cube;

		MyEffect effect;

		float rotation[3] = { };

		bool stop = false;

		while (!stop)
		{
			framework.process();

			// process input

			if (mouse.isDown(BUTTON_LEFT))
			{
				rotation[0] += mouse.dy / 100.f;
				rotation[1] += mouse.dx / 100.f;
			}

			// process audio

			audioOutput.Update(&audioStream);

			// generate FFT

			fftProcess();

			// evaluate cube

			effect.tick(framework.timeStep);

			evalCube(cube, &effect);

			// todo : send output towards hardware

			framework.beginDraw(0, 0, 0, 0);
			{
				// draw debug visualisation

				gxMatrixMode(GL_PROJECTION);
				gxPushMatrix();
				{
					Mat4x4 t;
					t.MakePerspectiveGL(M_PI/2.f, 1.f, .1f, 10.f);
					gxLoadMatrixf(t.m_v);

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
					setColor(colorWhite);
					gxBegin(GL_LINE_LOOP);
					{
						gxVertex2f(0.f, 400.f);

						for (int i = 0; i < kFFTComplexSize; ++i)
						{
							float p;
							p = s_fftReal[i] * s_fftReal[i] + s_fftImaginary[i] * s_fftImaginary[i];
							p = sqrtf(p);
							//p = 20.f * std::log10f(p);
						
							const float x = i / float(kFFTComplexSize - 1);
							const float y = p;
							gxVertex2f(x, y);
						}

						gxVertex2f(800.f, 400.f);
					}
					gxEnd();
				}
				gxPopMatrix();
			}
			framework.endDraw();
		}
	}

	return 0;
}
