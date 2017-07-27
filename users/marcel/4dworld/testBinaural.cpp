#include "audio.h"
#include "binaural.h"
#include "binaural_ircam.h"
#include "binaural_mit.h"
#include "framework.h"
#include "paobject.h"

using namespace binaural;

#define AUDIO_UPDATE_SIZE (AUDIO_BUFFER_SIZE/2)

#define BLEND_PREVIOUS_HRTF 1

#define OPTIMIZED_SAMPLE_COPIES 1

#define AUDIO_RAMPING_FIX 1

extern const int GFX_SX;
extern const int GFX_SY;

static SDL_mutex * g_audioMutex = nullptr;

static void drawHrirSampleGrid(const HRIRSampleSet & dataSet, const Vec2 & hoverLocation, const HRIRSampleGrid::Cell * hoverCell, const HRIRSampleGrid::Triangle * hoverTriangle);

struct BinauralObject
{
	struct SampleLocation
	{
		float elevation;
		float azimuth;
		
		SampleLocation()
			: elevation(0.f)
			, azimuth(0.f)
		{
		}
	};
	
	struct SampleBuffer
	{
		static const int kBufferSize = AUDIO_BUFFER_SIZE * 2;
		
		float samples[kBufferSize];
		int nextWriteIndex;
		int nextReadIndex;
		
		SampleBuffer()
			: nextWriteIndex(0)
			, nextReadIndex(0)
		{
		}
	};
	
	HRIRSampleSet * sampleSet;
	
	SampleBuffer sampleBuffer;
	
	float overlapBuffer[AUDIO_BUFFER_SIZE];
	
	SampleLocation sampleLocation;
	
#if BLEND_PREVIOUS_HRTF
	HRTF hrtfs[2];
	int nextHrtfIndex;
#endif

	AudioBuffer audioBufferL;
	AudioBuffer audioBufferR;
	int nextReadLocation;
	
	BinauralObject()
		: sampleSet(nullptr)
		, sampleBuffer()
		, overlapBuffer()
		, sampleLocation()
	#if BLEND_PREVIOUS_HRTF
		, hrtfs()
		, nextHrtfIndex(0)
	#endif
		, audioBufferL()
		, audioBufferR()
		, nextReadLocation(AUDIO_BUFFER_SIZE)
	{
	#if BLEND_PREVIOUS_HRTF
		memset(hrtfs, 0, sizeof(hrtfs));
	#endif
	}
	
	void init(HRIRSampleSet * _sampleSet)
	{
		sampleSet = _sampleSet;
	}
	
	void setSampleLocation(const float elevation, const float azimuth)
	{
		SDL_LockMutex(g_audioMutex);
		{
			sampleLocation.elevation = elevation;
			sampleLocation.azimuth = azimuth;
		}
		SDL_UnlockMutex(g_audioMutex);
	}
	
	void provide(const float * __restrict samples, const int numSamples)
	{
	#if OPTIMIZED_SAMPLE_COPIES
		int left = numSamples;
		int done = 0;
		
		while (left != 0)
		{
			if (sampleBuffer.nextWriteIndex == SampleBuffer::kBufferSize)
			{
				sampleBuffer.nextWriteIndex = 0;
			}
			
			const int todo = std::min(left, SampleBuffer::kBufferSize - sampleBuffer.nextWriteIndex);
			
			memcpy(sampleBuffer.samples + sampleBuffer.nextWriteIndex, samples + done, todo * sizeof(float));
			
			sampleBuffer.nextWriteIndex += todo;
			
			left -= todo;
			done += todo;
		}
	#else
		for (int i = 0; i < numSamples; ++i)
		{
			if (sampleBuffer.nextWriteIndex == SampleBuffer::kBufferSize)
			{
				sampleBuffer.nextWriteIndex = 0;
			}
			
			sampleBuffer.samples[sampleBuffer.nextWriteIndex] = samples[i];
			
			sampleBuffer.nextWriteIndex++;
		}
	#endif
	}
	
	void fillReadBuffer()
	{
		// move the old audio signal to the start of the overlap buffer
		
		memcpy(overlapBuffer, overlapBuffer + AUDIO_UPDATE_SIZE, (AUDIO_BUFFER_SIZE - AUDIO_UPDATE_SIZE) * sizeof(float));
		
		// generate audio signal
		
		float * __restrict samples = overlapBuffer + AUDIO_BUFFER_SIZE - AUDIO_UPDATE_SIZE;
		
	#if OPTIMIZED_SAMPLE_COPIES
		int left = AUDIO_UPDATE_SIZE;
		int done = 0;
		
		while (left != 0)
		{
			if (sampleBuffer.nextReadIndex == SampleBuffer::kBufferSize)
			{
				sampleBuffer.nextReadIndex = 0;
			}
			
			const int todo = std::min(left, SampleBuffer::kBufferSize - sampleBuffer.nextReadIndex);
			
			memcpy(samples + done, sampleBuffer.samples + sampleBuffer.nextReadIndex, todo * sizeof(float));
			
			sampleBuffer.nextReadIndex += todo;
			
			left -= todo;
			done += todo;
		}
	#else
		for (int i = 0; i < AUDIO_UPDATE_SIZE; ++i)
		{
			if (sampleBuffer.nextReadIndex == SampleBuffer::kBufferSize)
				sampleBuffer.nextReadIndex = 0;
			
			Assert(sampleBuffer.nextReadIndex != sampleBuffer.nextWriteIndex);
			
			samples[i] = sampleBuffer.samples[sampleBuffer.nextReadIndex];
			
			sampleBuffer.nextReadIndex++;
		}
	#endif
		
		// compute the HRIR, a blend between three sample points in a Delaunay triangulation of all sample points
		
		HRIRSampleData hrir;
		
		{
			const HRIRSampleData * samples[3];
			float sampleWeights[3];
			
			float elevation;
			float azimuth;
			
			SDL_LockMutex(g_audioMutex);
			{
				elevation = sampleLocation.elevation;
				azimuth = sampleLocation.azimuth;
			}
			SDL_UnlockMutex(g_audioMutex);
			
			if (sampleSet != nullptr && sampleSet->lookup_3(elevation, azimuth, samples, sampleWeights))
			{
				blendHrirSamples_3(samples, sampleWeights, hrir);
			}
			else
			{
				memset(&hrir, 0, sizeof(hrir));
			}
		}
		
		// compute the HRTF from the HRIR
		
	#if BLEND_PREVIOUS_HRTF
		const HRTF & oldHrtf = hrtfs[1 - nextHrtfIndex];
		HRTF & newHrtf = hrtfs[nextHrtfIndex];
		nextHrtfIndex = (nextHrtfIndex + 1) % 2;
	#else
		HRTF newHrtf;
	#endif
		
		hrirToHrtf(hrir.lSamples, hrir.rSamples, newHrtf.lFilter, newHrtf.rFilter);
		
		// prepare audio signal for HRTF application
		
		AudioBuffer audioBuffer;
		reverseSampleIndices(overlapBuffer, audioBuffer.real);
		memset(audioBuffer.imag, 0, AUDIO_BUFFER_SIZE * sizeof(float));
		
		// apply HRTF
		
		// convolve audio in the frequency domain
		
	#if BLEND_PREVIOUS_HRTF
		AudioBuffer oldAudioBufferL;
		AudioBuffer oldAudioBufferR;
		
		AudioBuffer newAudioBufferL;
		AudioBuffer newAudioBufferR;
		
		convolveAudio_2(
			audioBuffer,
			oldHrtf.lFilter,
			oldHrtf.rFilter,
			newHrtf.lFilter,
			newHrtf.rFilter,
			oldAudioBufferL,
			oldAudioBufferR,
			newAudioBufferL,
			newAudioBufferR);
		
		// ramp from old to new audio buffer
		
	#if AUDIO_RAMPING_FIX
		const int offset = AUDIO_BUFFER_SIZE - AUDIO_UPDATE_SIZE;
		
		rampAudioBuffers(oldAudioBufferL.real + offset, newAudioBufferL.real + offset, AUDIO_UPDATE_SIZE, audioBufferL.real + offset);
		rampAudioBuffers(oldAudioBufferR.real + offset, newAudioBufferR.real + offset, AUDIO_UPDATE_SIZE, audioBufferR.real + offset);
	#else
		rampAudioBuffers(oldAudioBufferL.real, newAudioBufferL.real, AUDIO_BUFFER_SIZE, audioBufferL.real);
		rampAudioBuffers(oldAudioBufferR.real, newAudioBufferR.real, AUDIO_BUFFER_SIZE, audioBufferR.real);
	#endif
	#else
		AudioBuffer tempL;
		AudioBuffer tempR;
		
		convolveAudio_2(
			audioBuffer,
			newHrtf.lFilter,
			newHrtf.rFilter,
			newHrtf.lFilter,
			newHrtf.rFilter,
			audioBufferL,
			audioBufferR,
			tempL,
			tempR);
	#endif
	
		nextReadLocation = AUDIO_BUFFER_SIZE - AUDIO_UPDATE_SIZE;
	}
	
	void generate(
		float * __restrict samples,
		const int numSamples)
	{
		for (int i = 0; i < numSamples; ++i)
		{
			if (nextReadLocation == AUDIO_BUFFER_SIZE)
			{
				fillReadBuffer();
			}
			
			samples[i * 2 + 0] = audioBufferL.real[nextReadLocation];
			samples[i * 2 + 1] = audioBufferR.real[nextReadLocation];
			
			nextReadLocation++;
		}
	}
};

struct PcmData
{
	float * samples;
	int numSamples;
	
	PcmData()
		: samples(nullptr)
		, numSamples(0)
	{
	}
	
	~PcmData()
	{
		delete samples;
		samples = nullptr;
	}
	
	void init(const char * filename)
	{
		::SoundData * sound = ::loadSound(filename);
		
		if (sound->sampleCount > 0 && sound->channelSize == 2)
		{
			samples = new float[sound->sampleCount];
			numSamples = sound->sampleCount;
			
			const short * __restrict sampleData = (short*)sound->sampleData;
			const float scale = 1.f / (1 << 15);
			
			for (int i = 0; i < numSamples; ++i)
				samples[i] = sampleData[i * sound->channelCount] * scale;
		}
	}
};

struct PcmObject
{
	const PcmData * pcmData;
	int nextSampleIndex;
	
	PcmObject()
		: pcmData(nullptr)
		, nextSampleIndex(0)
	{
	}
	
	void init(const PcmData * _pcmData)
	{
		pcmData = _pcmData;
	}
	
	void generate(float * __restrict samples, const int numSamples)
	{
		for (int i = 0; i < numSamples; ++i)
		{
			if (pcmData == nullptr || pcmData->numSamples == 0)
			{
				samples[i] = 0.f;
			}
			else
			{
				if (nextSampleIndex == pcmData->numSamples)
					nextSampleIndex = 0;
				
				samples[i] = pcmData->samples[nextSampleIndex];
				
				nextSampleIndex++;
			}
			
			//samples[i] = random(-.1f, +.1f);
		}
	}
};

struct BinauralSound
{
	BinauralObject binauralObject;
	PcmObject pcmObject;
	
	BinauralSound()
		: binauralObject()
		, pcmObject()
	{
	}
	
	void init(HRIRSampleSet * sampleSet, PcmData * pcmData)
	{
		binauralObject.init(sampleSet);
		
		pcmObject.init(pcmData);
	}
	
	void generate(float * __restrict samples)
	{
		float pcmSamples[AUDIO_UPDATE_SIZE];
		pcmObject.generate(pcmSamples, AUDIO_UPDATE_SIZE);
		
		binauralObject.provide(pcmSamples, AUDIO_UPDATE_SIZE);
		
		binauralObject.generate(samples, AUDIO_UPDATE_SIZE);
	}
};

struct MyPortAudioHandler : PortAudioHandler
{
	std::vector<BinauralSound*> sounds;
	
	float gain;
	
	MyPortAudioHandler()
		: sounds()
		, gain(1.f)
	{
	}
	
	~MyPortAudioHandler()
	{
		for (auto & sound : sounds)
		{
			delete sound;
			sound = nullptr;
		}
		
		sounds.clear();
	}
	
	void addBinauralSound(HRIRSampleSet * sampleSet, PcmData * pcmData)
	{
		BinauralSound * sound = new BinauralSound();
		
		sound->init(sampleSet, pcmData);
		
		sounds.push_back(sound);
	}
	
	virtual void portAudioCallback(
		const void * inputBuffer,
		void * outputBuffer,
		int framesPerBuffer) override
	{
		float * __restrict samples = (float*)outputBuffer;
		
		memset(samples, 0, framesPerBuffer * 2 * sizeof(float));
		
		for (auto & sound : sounds)
		{
			float soundSamples[AUDIO_UPDATE_SIZE * 2];
			
			sound->generate(soundSamples);
			
			for (int i = 0; i < AUDIO_UPDATE_SIZE * 2; ++i)
			{
				samples[i] += soundSamples[i];
			}
		}
		
		for (int i = 0; i < AUDIO_UPDATE_SIZE * 2; ++i)
		{
			samples[i] *= gain;
		}
	}
};

void testBinaural()
{
	fassert(g_audioMutex == nullptr);
	g_audioMutex = SDL_CreateMutex();
	
	if (false)
	{
		// try loading an IRCAM sample set

		HRIRSampleSet sampleSet;

		loadHRIRSampleSet_Ircam("binaural/IRC_1057", sampleSet);
	}
	
	if (false)
	{
		// try loading an MIT sample set

		HRIRSampleSet sampleSet;

		loadHRIRSampleSet_Mit("binaural/MIT-HRTF-DIFFUSE", sampleSet);
		
		sampleSet.finalize();
	}
	
	if (false)
	{
		// try loading a sample set and performing lookups

		HRIRSampleSet sampleSet;

		loadHRIRSampleSet_Mit("binaural/MIT-HRTF-DIFFUSE", sampleSet);
		
		sampleSet.finalize();
		
		for (int elevation = 0; elevation <= 180; elevation += 10)
		{
			for (int azimuth = 0; azimuth <= 360; azimuth += 10)
			{
				HRIRSampleData const * samples[3];
				float sampleWeights[3];
				
				if (sampleSet.lookup_3(elevation, azimuth, samples, sampleWeights) == false)
				{
					debugLog("sample set lookup failed! elevation=%d, azimuth=%d", elevation, azimuth);
				}
				else
				{
					debugTimerBegin("blend_hrir");
					
					HRIRSampleData weightedSample;
					
					blendHrirSamples_3(
						*samples[0], sampleWeights[0],
						*samples[1], sampleWeights[1],
						*samples[2], sampleWeights[2],
						weightedSample);
					
					debugTimerEnd("blend_hrir");
				}
			}
		}
	}
	
	{
		for (int i = 0; i < 100; ++i)
		{
			float elevation = random(-90.f, +90.f);
			float azimuth = random(-180.f, +180.f);
			float x, y, z;
			
			elevationAndAzimuthToCartesian(elevation, azimuth, x, y, z);
			debugLog("(%.2f, %.2f) -> (%.2f, %.2f, %.2f)", elevation, azimuth, x, y, z);
			
			cartesianToElevationAndAzimuth(x, y, z, elevation, azimuth);
			debugLog("(%.2f, %.2f) -> (%.2f, %.2f, %.2f)", elevation, azimuth, x, y, z);
			
			elevationAndAzimuthToCartesian(elevation, azimuth, x, y, z);
			debugLog("(%.2f, %.2f) -> (%.2f, %.2f, %.2f)", elevation, azimuth, x, y, z);
			
			cartesianToElevationAndAzimuth(x, y, z, elevation, azimuth);
			debugLog("(%.2f, %.2f) -> (%.2f, %.2f, %.2f)", elevation, azimuth, x, y, z);
			
			debugLog("----");
		}
	}
	
	{
		// try loading a sample set and displaying it

		HRIRSampleSet sampleSet;

		loadHRIRSampleSet_Mit("binaural/MIT-HRTF-DIFFUSE", sampleSet);
		
		//loadHRIRSampleSet_Ircam("binaural/IRC_1057", sampleSet);
		
		sampleSet.finalize();
		
		float scale = 1.f;
		Vec2 translation;
		
	#if BLEND_PREVIOUS_HRTF
		HRTF previousHrtf;
		memset(&previousHrtf, 0, sizeof(previousHrtf));
	#endif
		
		float overlapBuffer[AUDIO_BUFFER_SIZE];
		memset(&overlapBuffer, 0, sizeof(overlapBuffer));
		
		float oscillatorPhase = 0.f;
		
		Surface view3D(200, 200, false);
		
		PcmData pcmData;
		pcmData.init("music2.ogg");
		
		MyPortAudioHandler audio;
		int numSources = 0;
		//for (int i = 0; i < 100; ++i)
		for (int i = 0; i < 1; ++i)
		{
			audio.addBinauralSound(&sampleSet, &pcmData);
			
			numSources++;
		}
		
		audio.gain = 1.f / numSources;
		
		PortAudioObject pa;
		pa.init(44100, 2, AUDIO_UPDATE_SIZE, &audio);
		
		do
		{
			framework.process();
			
			//
			
			if (mouse.isDown(BUTTON_LEFT))
			{
				if (mouse.dy > 0.f)
					scale *= 1.f + std::abs(float(mouse.dy)) / 100.f;
				else
					scale /= 1.f + std::abs(float(mouse.dy)) / 100.f;
			}
			
			if (keyboard.isDown(SDLK_LSHIFT) || keyboard.isDown(SDLK_RSHIFT))
			{
				translation[0] += mouse.dx / scale;
				translation[1] -= mouse.dy / scale;
			}
			
			const Mat4x4 transform =
				Mat4x4(true).
				Translate(GFX_SX/2, GFX_SY/2, 0).
				Scale(+scale, -scale, 1.f).
				Translate(translation[0], translation[1], 0);
			
			// mouse picking
			
			const Vec2 hoverLocation = transform.Invert() * Vec2(mouse.x, mouse.y);
			
			SDL_LockMutex(g_audioMutex);
			{
				for (auto & sound : audio.sounds)
				{
					sound->binauralObject.setSampleLocation(hoverLocation[1], hoverLocation[0]);
				}
			}
			SDL_UnlockMutex(g_audioMutex);
			
			float baryU;
			float baryV;
			
			auto hoverCell = sampleSet.sampleGrid.lookupCell(hoverLocation[1], hoverLocation[0]);
			auto hoverTriangle = sampleSet.sampleGrid.lookupTriangle(hoverLocation[1], hoverLocation[0], baryU, baryV);
			
			// generate audio signal
			
			memcpy(overlapBuffer, overlapBuffer + AUDIO_UPDATE_SIZE, (AUDIO_BUFFER_SIZE - AUDIO_UPDATE_SIZE) * sizeof(float));
			
			float * __restrict samples = overlapBuffer + AUDIO_BUFFER_SIZE - AUDIO_UPDATE_SIZE;
			
			float oscillatorPhaseStep = 1.f / 50.f;
			const float twoPi = M_PI * 2.f;
			
			for (int i = 0; i < AUDIO_UPDATE_SIZE; ++i)
			{
				samples[i] = std::sin(oscillatorPhase * twoPi);
				
				oscillatorPhase = std::fmod(oscillatorPhase + oscillatorPhaseStep, 1.f);
			}
			
			// compute the HRIR, a blend between three sample points in a Delaunay triangulation of all sample points
			
			HRIRSampleData hrir;
			
			{
				const HRIRSampleData * samples[3];
				float sampleWeights[3];
				
				if (sampleSet.lookup_3(hoverLocation[1], hoverLocation[0], samples, sampleWeights))
				{
					blendHrirSamples_3(samples, sampleWeights, hrir);
				}
				else
				{
					memset(&hrir, 0, sizeof(hrir));
				}
			}
			
			// compute the HRTF from the HRIR
			
			HRTF hrtf;
			
			hrirToHrtf(hrir.lSamples, hrir.rSamples, hrtf.lFilter, hrtf.rFilter);
			
			// prepare audio signal for HRTF application
			
			AudioBuffer audioBuffer;
			reverseSampleIndices(overlapBuffer, audioBuffer.real);
			memset(audioBuffer.imag, 0, AUDIO_BUFFER_SIZE * sizeof(float));
			
			// apply HRTF
			
			AudioBuffer audioBufferL;
			AudioBuffer audioBufferR;
			
		#if 0
			audioBufferL = audioBuffer;
			audioBufferR = audioBuffer;
		#elif BLEND_PREVIOUS_HRTF
			// convolve audio in the frequency domain
			
			const HRTF & oldHrtf = previousHrtf;
			const HRTF & newHrtf = hrtf;
			
			AudioBuffer oldAudioBufferL;
			AudioBuffer oldAudioBufferR;
			
			AudioBuffer newAudioBufferL;
			AudioBuffer newAudioBufferR;
			
			convolveAudio_2(
				audioBuffer,
				oldHrtf.lFilter,
				oldHrtf.rFilter,
				newHrtf.lFilter,
				newHrtf.rFilter,
				oldAudioBufferL,
				oldAudioBufferR,
				newAudioBufferL,
				newAudioBufferR);
			
			// ramp from old to new audio buffer
			
			debugTimerBegin("rampAudioBuffers");
			
			rampAudioBuffers(oldAudioBufferL.real, newAudioBufferL.real, AUDIO_BUFFER_SIZE, audioBufferL.real);
			rampAudioBuffers(oldAudioBufferR.real, newAudioBufferR.real, AUDIO_BUFFER_SIZE, audioBufferR.real);
			
			debugTimerEnd("rampAudioBuffers");
			
			previousHrtf = hrtf;
		#else
			// convolve audio in the frequency domain
			
			convolveAudio(
				audioBuffer,
				hrtf.lFilter,
				hrtf.rFilter,
				audioBufferL,
				audioBufferR);
		#endif
			
			//
			
			framework.beginDraw(230, 230, 230, 0);
			{
				setFont("calibri.ttf");
				pushFontMode(FONT_SDF);
				const int fontSize = 14;
				
				//
				
				gxPushMatrix();
				{
					gxMultMatrixf(transform.m_v);
					
					drawHrirSampleGrid(sampleSet, hoverLocation, hoverCell, hoverTriangle);
				}
				gxPopMatrix();
				
				pushSurface(&view3D);
				{
					view3D.clear(200, 200, 200);
					
					Mat4x4 matP;
					Mat4x4 matC;
					Mat4x4 matV;
					Mat4x4 matO;
					
					matP.MakePerspectiveLH(M_PI/2.f, 1.f, .001f, 10.f);
					matC.MakeLookat(Vec3(0.f, 0.f, 0.f), Vec3(1.f, 0.f, 0.f), Vec3(0.f, 1.f, 0.f));
					matC = matC.Scale(1, -1, 1).Translate(0.f, 0.f, -2.f);
					matV = matC.Invert();
					matO = Mat4x4(true).RotateY(framework.time * .1f);
					
					const Mat4x4 mat = matP * matV;
					gxMatrixMode(GL_PROJECTION);
					gxPushMatrix();
					gxLoadMatrixf(mat.m_v);
					gxMatrixMode(GL_MODELVIEW);
					gxPushMatrix();
					gxLoadMatrixf(matO.m_v);
					
					glPointSize(2.f);
					gxBegin(GL_POINTS);
					{
						for (int elevation = -90; elevation < +90; elevation += 10)
						{
							for (int azimuth = -180; azimuth < +180; azimuth += 10)
							{
								float x, y, z;
								elevationAndAzimuthToCartesian(elevation, azimuth, x, y, z);
								
								setColor(150, 150, 150);
								gxVertex3f(x, y, z);
							}
						}
					}
					gxEnd();
					glPointSize(1.f);
					
					gxBegin(GL_LINES);
					{
						gxColor4f(1, 0, 0, 1); gxVertex3f(0, 0, 0); gxVertex3f(1, 0, 0);
						gxColor4f(0, 1, 0, 1); gxVertex3f(0, 0, 0); gxVertex3f(0, 1, 0);
						gxColor4f(0, 0, 1, 1); gxVertex3f(0, 0, 0); gxVertex3f(0, 0, 1);
					}
					gxEnd();
					
					gxMatrixMode(GL_PROJECTION);
					gxPopMatrix();
					gxMatrixMode(GL_MODELVIEW);
					gxPopMatrix();
				}
				popSurface();
				
				gxPushMatrix();
				{
					gxTranslatef(10, GFX_SY - view3D.getHeight() - 50 - 10, 0);
					gxSetTexture(view3D.getTexture());
					{
						pushBlend(BLEND_OPAQUE);
						setColor(colorWhite);
						drawRect(0, 0, view3D.getWidth(), view3D.getHeight());
						popBlend();
					}
					gxSetTexture(0);
				}
				gxPopMatrix();
				
				//
				
				gxPushMatrix();
				{
					const int sx = HRIR_BUFFER_SIZE;
					const int sy = 50;
					
					setColor(colorBlack);
					drawRect(0, 0, sx, sy);
					
					pushBlend(BLEND_ADD);
					{
						setColor(colorRed);
						for (int i = 0; i < sx; ++i)
						{
							const float v = .5f + hrir.lSamples[i];
							setColorf(v, 0.f, v / 4.f);
							drawLine(i, 0, i, sy/2 + hrir.lSamples[i] * sy/2);
						}
						
						setColor(colorGreen);
						for (int i = 0; i < sx; ++i)
						{
							const float v = .5f + hrir.rSamples[i];
							setColorf(0.f, v, v / 4.f);
							drawLine(i, 0, i, sy/2 + hrir.rSamples[i] * sy/2);
						}
					}
					popBlend();
					
					setColor(50, 50, 50);
					drawText(5, 5, fontSize, +1, +1, "HRIR left & right ear");
				}
				gxPopMatrix();
				
				//
				
				gxPushMatrix();
				{
					gxTranslatef(GFX_SX - AUDIO_BUFFER_SIZE, 0, 0);
					
					const int sx = AUDIO_BUFFER_SIZE;
					const int sy = 50;
					
					setColor(colorBlack);
					drawRect(0, 0, sx, sy);
					
					pushBlend(BLEND_ADD);
					{
						setColor(colorRed);
						for (int i = 0; i < sx; ++i)
						{
							setColorf(1.f, 0.f, .5f);
							drawLine(i, 0, i, sy/2 + audioBufferL.real[i] * sy/2);
						}
						
						setColor(colorGreen);
						for (int i = 0; i < sx; ++i)
						{
							setColorf(0.f, 1.f, .5f);
							drawLine(i, 0, i, sy/2 + audioBufferR.real[i] * sy/2);
						}
					}
					popBlend();
					
					setColor(50, 50, 50);
					drawText(5, 5, fontSize, +1, +1, "audio left & right ear (stereo) ");
				}
				gxPopMatrix();
				
				//
				
				gxPushMatrix();
				{
					gxTranslatef(GFX_SX - AUDIO_BUFFER_SIZE, 50, 0);
					
					const int sx = AUDIO_BUFFER_SIZE;
					const int sy = 50;
					
					setColor(colorBlack);
					drawRect(0, 0, sx, sy);
					
					pushBlend(BLEND_ADD);
					{
						setColor(colorRed);
						for (int i = 0; i < sx; ++i)
						{
							setColorf(1., 0.f, .5f);
							drawLine(i, 0, i, sy/2 + overlapBuffer[i] * sy/2);
						}
					}
					popBlend();
					
					setColor(230, 230, 230);
					drawText(5, 5, fontSize, +1, +1, "source audio (mono)");
				}
				gxPopMatrix();
				
				//
				
				gxPushMatrix();
				{
					gxTranslatef(0, GFX_SY - 50, 0);
					
					const int sx = HRTF_BUFFER_SIZE;
					const int sy = 50;
					
					setColor(colorBlack);
					drawRect(0, 0, sx, sy);
					pushBlend(BLEND_ADD);
					for (int i = 0; i < sx; ++i)
					{
						const int j = (i + sx/2) % sx;
						const float power = std::hypotf(hrtf.lFilter.real[j], hrtf.lFilter.imag[j]);
						setColorf(1.f, 0.f, 0.f, power);
						drawLine(i, 0, i, sy);
					}
					popBlend();
					
					setColor(230, 230, 230);
					drawText(5, 5, fontSize, +1, +1, "HRTF (left ear)");
				}
				gxPopMatrix();
				
				gxPushMatrix();
				{
					gxTranslatef(GFX_SX - HRIR_BUFFER_SIZE, GFX_SY - 50, 0);
					
					const int sx = HRTF_BUFFER_SIZE;
					const int sy = 50;
					
					setColor(colorBlack);
					drawRect(0, 0, sx, sy);
					pushBlend(BLEND_ADD);
					for (int i = 0; i < sx; ++i)
					{
						const int j = (i + sx/2) % sx;
						const float power = std::hypotf(hrtf.rFilter.real[j], hrtf.rFilter.imag[j]);
						setColorf(0.f, 1.f, 0.f, power);
						drawLine(i, 0, i, sy);
					}
					popBlend();
					
					setColor(230, 230, 230);
					drawText(5, 5, fontSize, +1, +1, "HRTF (right ear)");
				}
				gxPopMatrix();
				
				
				gxPushMatrix();
				{
					gxTranslatef((GFX_SX - AUDIO_BUFFER_SIZE) / 2, GFX_SY - 50, 0);
					
					float samples0[AUDIO_BUFFER_SIZE];
					float samples1[AUDIO_BUFFER_SIZE];
					float samples[AUDIO_BUFFER_SIZE];
					
					for (int i = 0; i < AUDIO_BUFFER_SIZE; ++i)
					{
						samples0[i] = 0.f;
						samples1[i] = 1.f;
						samples[i] = random(0.f, 1.f);
					}
					
					rampAudioBuffers(samples0, samples1, AUDIO_BUFFER_SIZE, samples);
					
					const int sx = AUDIO_BUFFER_SIZE;
					const int sy = 50;
					
					setColor(colorBlack);
					drawRect(0, 0, sx, sy);
					pushBlend(BLEND_ADD);
					for (int i = 0; i < sx; ++i)
					{
						setColorf(1.f, 0.f, 0.f);
						drawLine(i, 0, i, sy * samples0[i]);
						
						setColorf(0.f, 1.f, 0.f);
						drawLine(i, 0, i, sy * samples1[i]);
						
						setColorf(0.f, 0.f, 1.f);
						drawLine(i, 0, i, sy * samples[i]);
					}
					popBlend();
					
					setColor(230, 230, 230);
					drawText(5, 5, fontSize, +1, +1, "HRTF (right ear)");
				}
				gxPopMatrix();
				
				//
				
				for (int offset = 1; offset >= 0; --offset)
				{
					gxPushMatrix();
					gxTranslatef(offset, offset, 0);
					
					setColor(offset == 0 ? colorWhite : Color(0, 0, 0, 127));
					
					drawText(mouse.x, mouse.y + 20, fontSize, 0, 1, "azimuth=%.2f, elevation=%.2f", hoverLocation[0], hoverLocation[1]);
					
					if (hoverTriangle != nullptr)
					{
						drawText(mouse.x, mouse.y + 40, fontSize, 0, 1, "bary=(%.2f, %.2f, %.2f)", baryU, baryV, 1.f - baryU - baryV);
					}
					
					if (hoverCell != nullptr)
					{
						drawText(mouse.x, mouse.y + 60, fontSize, 0, 1, "cell.numTriangles=%d", int(hoverCell->triangles.size()));
					}
					
					gxPopMatrix();
				}
				
				popFontMode();
			}
			framework.endDraw();
		} while (!keyboard.wentDown(SDLK_SPACE));
		
		pa.shut();
	}
	
	fassert(g_audioMutex != nullptr);
	SDL_DestroyMutex(g_audioMutex);
	g_audioMutex = nullptr;
}

static void drawHrirSampleGrid(const HRIRSampleSet & sampleSet, const Vec2 & hoverLocation, const HRIRSampleGrid::Cell * hoverCell, const HRIRSampleGrid::Triangle * hoverTriangle)
{
	hqBegin(HQ_FILLED_TRIANGLES);
	{
		int index = 0;
		
		for (auto & triangle : sampleSet.sampleGrid.triangles)
		{
			auto & p1 = triangle.vertex[0].location;
			auto & p2 = triangle.vertex[1].location;
			auto & p3 = triangle.vertex[2].location;
			
			bool isInHoverCell = false;
			
			if (hoverCell != nullptr)
			{
				for (auto cellTriangle : hoverCell->triangles)
					if (&triangle == cellTriangle)
						isInHoverCell = true;
			}
			
			if (&triangle == hoverTriangle)
			{
				setColor(colorYellow);
			}
			else if (isInHoverCell)
			{
				setColor(200, 200, 255);
			}
			else
			{
				setColorf(
					.5f + p1.azimuth   / 360.f,
					.5f + p1.elevation / 180.f,
					1.f);
			}
			
			hqFillTriangle(p2.azimuth, p2.elevation, p1.azimuth, p1.elevation, p3.azimuth, p3.elevation);
			
			index++;
		}
	}
	hqEnd();
	
	gxBegin(GL_POINTS);
	{
		for (auto & sample : sampleSet.samples)
		{
			setColor(colorWhite);
			gxVertex2f(sample->azimuth, sample->elevation);
		}
	}
	gxEnd();
}
