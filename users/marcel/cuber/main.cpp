#include "Calc.h"
#include "config.h"
#include "cube.h"
#include "fft.h"
#include "framework.h"
#include "Path.h"
#include "script.h"

#include "audiooutput/AudioOutput_Native.h"
#include "audiostream/AudioStreamVorbis.h"

#if defined(WIN32)
	#include <Windows.h>
#endif

#if !ENABLE_SCRIPT_EFFECT
	#include "script.cpp"
#endif

//

#if ENABLE_SCRIPT_EFFECT

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
			(void)e;
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
			(void)e;
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

#endif

//

static void evalCube(Cube & cube, Effect * effect)
{
	const float coordScale = 1.f / (SX - 1.f);

	#pragma omp parallel for
	for (int x = 0; x < SX; ++x)
	{
		for (int y = 0; y < SY; ++y)
		{
			for (int z = 0; z < SZ; ++z)
			{
				Coord c;

				//c.x = (x / (SX - 1.f) - .5f) * 2.f;
				//c.y = (y / (SY - 1.f) - .5f) * 2.f;
				//c.z = (z / (SZ - 1.f) - .5f) * 2.f;
				c.x = (x - (SX - 1) / 2.f) * coordScale;
				c.y = (y - (SY - 1) / 2.f) * coordScale;
				c.z = (z - (SZ - 1) / 2.f) * coordScale;

				cube.m_value[x][y][z] = effect->evaluate(c);

				const float d = 0.05f;
				const float c0 = (effect->evaluate(c + Coord(+d, 0.f, 0.f)) - effect->evaluate(c + Coord(-d, 0.f, 0.f)));
				const float c1 = (effect->evaluate(c + Coord(0.f, +d, 0.f)) - effect->evaluate(c + Coord(0.f, -d, 0.f)));
				const float c2 = (effect->evaluate(c + Coord(0.f, 0.f, +d)) - effect->evaluate(c + Coord(0.f, 0.f, -d)));
				const float cs = sqrtf(c0 * c0 + c1 * c1 + c2 * c2);
				cube.m_color[x][y][z][0] = ((c0 / cs) + 1.f) * .5f;
				cube.m_color[x][y][z][1] = ((c1 / cs) + 1.f) * .5f;
				cube.m_color[x][y][z][2] = ((c2 / cs) + 1.f) * .5f;

				effect->evaluateRaw(x, y, z, cube.m_value[x][y][z]);
			}
		}
	}
}

static void drawCube(const Cube & cube)
{
	const float coordScale = 1.f / SX;

	gxPushMatrix();
	{
		gxScalef(
			coordScale,
			coordScale,
			coordScale);

		gxTranslatef(
			-(SX - 1) / 2.f,
			-(SY - 1) / 2.f,
			-(SZ - 1) / 2.f);

	#if 1
		// the most beautiful way ever to draw the edges of a cube..
		gxBegin(GX_LINES);
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

		setBlend(BLEND_ADD);
		beginCubeBatch();
		{
		#if VIDEO_RECORDING_MODE
			const Vec3 cubeSize(.04f, .04f, .04f);
		#else
			const Vec3 cubeSize(.02f, .02f, .02f);
		#endif
			
			for (int x = 0; x < SX; ++x)
			{
				for (int y = 0; y < SY; ++y)
				{
					for (int z = 0; z < SZ; ++z)
					{
						const float value = cube.m_value[x][y][z];

					#if 1
						gxColor4f(value, value, value, 1.f);
					#else
						gxColor4f(
							value * cube.m_color[x][y][z][0],
							value * cube.m_color[x][y][z][1],
							value * cube.m_color[x][y][z][2], 1.f);
					#endif
					
						fillCube(Vec3(x, y, z), cubeSize);
					}
				}
			}
		}
		endCubeBatch();

		setBlend(BLEND_ALPHA);
	}
	gxPopMatrix();
}

SDL_Window * getWindow();

static void drawCubeSlices(const Cube & cube)
{
	uint8_t * slices = new uint8_t[SX * SY * SZ * 4];

	const int sx = SX * SZ;
	const int sy = SY;

	const int pitch = sx;

	for (int x = 0; x < SX; ++x)
	{
		for (int y = 0; y < SY; ++y)
		{
			for (int z = 0; z < SZ; ++z)
			{
				float value = cube.m_value[x][y][z];

				//value += z / float(SZ);

				const float cx = x / float(SX - 1) - .5f;
				const float cy = y / float(SY - 1) - .5f;
				const float cz = z / float(SZ - 1) - .5f;

				const Coord c(cx, cy, cz);

				const float r = value * cube.m_color[x][y][z][0];
				const float g = value * cube.m_color[x][y][z][1];
				const float b = value * cube.m_color[x][y][z][2];

				//const float r = value + powf(computePerlinNoise(c, x + framework.time), 3.f);
				//const float g = value + powf(computePerlinNoise(c, y + framework.time), 3.f);
				//const float b = value + powf(computePerlinNoise(c, z + framework.time), 3.f);

				const int tx = x + z * SX;
				const int ty = y;

				const int index = (tx + ty * pitch) * 4;

				slices[index + 0] = clamp(int(r * 255.f), 0, 255);
				slices[index + 1] = clamp(int(g * 255.f), 0, 255);
				slices[index + 2] = clamp(int(b * 255.f), 0, 255);
				slices[index + 3] = 255;
			}
		}
	}

	// send output towards hardware by blitting it to the screen!
	
#if ENABLE_OPENGL || ENABLE_METAL
	GxTextureId texture = createTextureFromRGBA8(slices, SX * SZ, SY, true, true);

	if (texture)
	{
		gxSetTexture(texture, GX_SAMPLE_LINEAR, true);
		gxPushMatrix();
		{
			gxTranslatef(73, 52, 0);
			gxColor4f(1.f, 1.f, 1.f, 1.f);
			drawRect(0, 0, SX * SZ, SY);
		}
		gxPopMatrix();
		gxClearTexture();

		freeTexture(texture);
	}
#else
	SDL_Surface * surface = getWindowSurface();

	if (SDL_LockSurface(surface) == 0)
	{
		uint8_t * pixels = (uint8_t*)surface->pixels;

		//for (int x = 0; x < sx; ++x)
		//{
			for (int y = 0; y < sy; ++y)
			{
				const int tx = 52;
				const int ty = 73 + y;

				memcpy(pixels + surface->pitch * ty + tx * 4, slices + y * pitch * 4, pitch * 4);
			}
		//}

		SDL_UnlockSurface(surface);
	}

	SDL_UpdateWindowSurface(getWindow());
#endif

	delete [] slices;
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
			const float scale = 1.f / 65536.f;

			for (int i = 0; i < copySize; ++i)
			{
				const int value = buffer[i].channel[0] + buffer[i].channel[1];
				s_fftInputBuffer[i] = value * scale;
			}
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

#if defined(WIN32)

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

		// todo # let the user select a device

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

#endif

int main(int argc, char * argv[])
{
	setupPaths(CHIBI_RESOURCE_PATHS);

#if USE_AUDIO_INPUT
	AudioIn audioIn;

	audioIn.init(2, 44100, 4096 * 2);
#endif

	framework.windowX = 0;
	framework.windowY = 0;

	if (!framework.init(1100, 800))
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

		AudioOutput_Native audioOutput;
		audioOutput.Initialize(2, audioStreamOGG.SampleRate_get(), 256);
		audioOutput.Volume_set(1.f);
		//audioOutput.Play(&audioStream);
		audioOutput.Play(&audioStreamOGG); // note : bypass AudioStream_Capture for now, as the fft capture/proxy is a work in progress for now
	#endif

		Cube cube;

		EffectCtxImpl ctx;
	#if ENABLE_SCRIPT_EFFECT
	#ifdef DEBUG
		ScriptEffect effect(ctx, "Debug/script.dll");
	#else
		ScriptEffect effect(ctx, "Release/script.dll");
	#endif
	#else
		MyEffect effect(ctx);
	#endif

		float rotation[3] = { };

		bool stop = false;

		while (!stop)
		{
			framework.process();

			// process input

			if (keyboard.isDown(SDLK_ESCAPE))
				stop = true;

			if (mouse.isDown(BUTTON_LEFT))
			{
				rotation[0] += mouse.dy / 100.f;
				rotation[1] -= mouse.dx / 100.f;
			}

			// process audio

		#if USE_AUDIO_INPUT
			short buffer[kFFTBufferSize * 2];
			int sampleCount = 0;
			if (audioIn.provide(buffer, sampleCount))
			{
				const float scale = 2.f / 65536.f;
				for (int i = 0; i < kFFTBufferSize; ++i)
					s_fftInputBuffer[i] = buffer[i * 2] * scale;
				s_fftProvideTime = framework.time;
			}
		#else
			audioOutput.Update();
			
			for (int i = 0; i < kFFTBufferSize; ++i)
				s_fftInputBuffer[i] = random(-.1f, +.1f);
		#endif

			// generate FFT

			fftProcess(framework.time);

			// evaluate cube

			effect.tick(framework.timeStep);

			evalCube(cube, &effect);

			framework.beginDraw(0, 0, 0, 0);
			{
			#if ENABLE_OPENGL || ENABLE_METAL
				// draw debug visualisation

				setFont("calibri.ttf");

				gxMatrixMode(GX_PROJECTION);
				gxPushMatrix();
				{
					Mat4x4 t;
					t.MakePerspectiveGL(float(M_PI)/2.f, 1.f, .1f, 10.f);
					gxLoadMatrixf(t.m_v);
					gxScalef(1.f, -1.f, 1.f);

					gxMatrixMode(GX_MODELVIEW);
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
				gxMatrixMode(GX_PROJECTION);
				gxPopMatrix();

				gxMatrixMode(GX_MODELVIEW);

				gxPushMatrix();
				{
					gxTranslatef(0.f, 750.f, 0.f);
					gxScalef(800.f, -10.f, 1.f);
					//gxScalef(2400.f, -10.f, 1.f);
					setColor(colorWhite);
					gxBegin(GX_LINE_LOOP);
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
			#endif

				drawCubeSlices(cube);
			}
			framework.endDraw();
		}

		framework.shutdown();
	}

	return 0;
}
