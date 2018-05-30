/*
	Copyright (C) 2017 Marcel Smit
	marcel303@gmail.com
	https://www.facebook.com/marcel.smit981

	Permission is hereby granted, free of charge, to any person
	obtaining a copy of this software and associated documentation
	files (the "Software"), to deal in the Software without
	restriction, including without limitation the rights to use,
	copy, modify, merge, publish, distribute, sublicense, and/or
	sell copies of the Software, and to permit persons to whom the
	Software is furnished to do so, subject to the following
	conditions:

	The above copyright notice and this permission notice shall be
	included in all copies or substantial portions of the Software.

	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
	EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
	OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
	NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
	HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
	WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
	FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
	OTHER DEALINGS IN THE SOFTWARE.
*/

#include "audio.h"
#include "audioTypes.h"
#include "binaural.h"
#include "binaural_cipic.h"
#include "binaural_ircam.h"
#include "binaural_mit.h"
#include "binauralizer.h"
#include "framework.h"
#include "paobject.h"
#include "soundmix.h"
#include "Timer.h"
#include <algorithm>
#include <atomic>
#include <cmath>

#include "../libparticle/ui.h"

using namespace binaural;

#define FULLSCREEN 0

#define BLEND_PREVIOUS_HRTF 1
#define NUM_BINAURAL_SOUNDS 1

#define DO_VIEW3D 1

const int GFX_SX = 1300;
const int GFX_SY = 760;

static void drawHrirSampleGrid(const HRIRSampleSet & dataSet, const Vec2 & hoverLocation, const HRIRSampleGrid::Cell * hoverCell, const HRIRSampleGrid::Triangle * hoverTriangle);

namespace BinauralTestNamespace
{
	static SDL_mutex * s_audioMutexSDL = nullptr;
	
	struct AudioMutex : binaural::Mutex
	{
		virtual void lock() override
		{
			const int r = SDL_LockMutex(s_audioMutexSDL);
			Assert(r == 0);
		}
		
		virtual void unlock() override
		{
			const int r = SDL_UnlockMutex(s_audioMutexSDL);
			Assert(r == 0);
		}
	};
	
	static AudioMutex s_audioMutex;
	
	//

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
			if (pcmData == nullptr || pcmData->numSamples == 0)
			{
				memset(samples, 0, numSamples * sizeof(float));
			}
			else
			{
				int left = numSamples;
				int done = 0;
				
				while (left != 0)
				{
					if (nextSampleIndex == pcmData->numSamples)
					{
						nextSampleIndex = 0;
					}
					
					const int todo = std::min(left, pcmData->numSamples - nextSampleIndex);
					
					memcpy(samples + done, pcmData->samples + nextSampleIndex, todo * sizeof(float));
					
					nextSampleIndex += todo;
					
					left -= todo;
					done += todo;
				}
			}
			
			//samples[i] = random(-.1f, +.1f);
		}
	};

	struct BinauralSound
	{
		Binauralizer binauralizer;
		PcmObject pcmObject;
		
		BinauralSound()
			: binauralizer()
			, pcmObject()
		{
		}
		
		void init(HRIRSampleSet * sampleSet, PcmData * pcmData)
		{
			binauralizer.init(sampleSet, &s_audioMutex);
			
			pcmObject.init(pcmData);
		}
		
		void generate(float * __restrict samples)
		{
			float pcmSamples[AUDIO_UPDATE_SIZE];
			pcmObject.generate(pcmSamples, AUDIO_UPDATE_SIZE);
			
			binauralizer.provide(pcmSamples, AUDIO_UPDATE_SIZE);
			
			binauralizer.generateInterleaved(samples, AUDIO_UPDATE_SIZE);
		}
	};

	struct MyPortAudioHandler : PortAudioHandler
	{
		std::vector<BinauralSound*> sounds;
		
		float gain;
		
		std::atomic<uint64_t> msecsPerTick;
		std::atomic<uint64_t> msecsPerSecondAccu;
		std::atomic<uint64_t> msecsPerSecond;
		int numTicks;
		
		MyPortAudioHandler()
			: sounds()
			, gain(1.f)
			, msecsPerTick(0)
			, msecsPerSecondAccu(0)
			, msecsPerSecond(0)
			, numTicks(0)
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
			const int numInputChannels,
			void * outputBuffer,
			const int framesPerBuffer) override
		{
			Assert(framesPerBuffer == AUDIO_UPDATE_SIZE);
			
			const auto t1 = g_TimerRT.TimeUS_get();
			
			float * __restrict samples = (float*)outputBuffer;
			
			memset(samples, 0, framesPerBuffer * 2 * sizeof(float));
			
			for (auto & sound : sounds)
			{
				float soundSamples[AUDIO_UPDATE_SIZE * 2];
				
				sound->generate(soundSamples);
				
				audioBufferAdd(samples, soundSamples, AUDIO_UPDATE_SIZE * 2);
			}
			
			audioBufferMul(samples, AUDIO_UPDATE_SIZE * 2, gain);
			
			const auto t2 = g_TimerRT.TimeUS_get();
			
			const auto t = t2 - t1;
			msecsPerTick = t;
			msecsPerSecondAccu += t;
			
			numTicks++;
			
			if ((numTicks % (SAMPLE_RATE / AUDIO_UPDATE_SIZE)) == 0)
			{
				msecsPerSecond = msecsPerSecondAccu.load();
				msecsPerSecondAccu = 0;
			}
		}
	};
}

using namespace BinauralTestNamespace;

int main(int argc, char * argv[])
{
#if 0
	char * basePath = SDL_GetBasePath();
	changeDirectory(basePath);
	changeDirectory("data");
	SDL_free(basePath);
#endif

#if FULLSCREEN
	framework.fullscreen = true;
#endif
	
	if (!framework.init(0, 0, GFX_SX, GFX_SY))
		return -1;
	
	initUi();
	
	fassert(s_audioMutexSDL == nullptr);
	s_audioMutexSDL = SDL_CreateMutex();
	
	enableDebugLog = true;
	
	{
		// try loading a sample set and displaying it

		HRIRSampleSet sampleSet;
		
		loadHRIRSampleSet_Cipic("binaural/CIPIC/subject147", sampleSet);
		
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
		
	#if DO_VIEW3D
		Surface view3D(200, 200, false);
	#endif
		
		PcmData pcmData;
		pcmData.load("testsounds/birdTest.ogg", 0, false);
		
		MyPortAudioHandler audioHandler;
		int numSources = 0;
		
		for (int i = 0; i < NUM_BINAURAL_SOUNDS; ++i)
		{
			audioHandler.addBinauralSound(&sampleSet, &pcmData);
			
			numSources++;
		}
		
		if (numSources == 0)
			audioHandler.gain = 0.f;
		else
			audioHandler.gain = 1.f / numSources;
		
		PortAudioObject pa;
		pa.init(SAMPLE_RATE, 2, 0, AUDIO_UPDATE_SIZE, &audioHandler);
		
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
			
			s_audioMutex.lock();
			{
				int index = 0;
				
				for (auto & sound : audioHandler.sounds)
				{
				#if 0
					const float elevation = hoverLocation[1] + std::sin(framework.time * index / 98.f) * 60.f;
					index++;
					const float azimuth = hoverLocation[0] + std::cos(framework.time * index / 87.f) * 60.f;
					index++;
				#else
					const float elevation = hoverLocation[1] + (index <= 1 ? 0.f : std::sin(framework.time * index / 98.f) * 60.f);
					index++;
					const float azimuth = hoverLocation[0] + (index <= 1 ? 0.f : std::cos(framework.time * index / 87.f) * 60.f);
					index++;
				#endif
					
					sound->binauralizer.setSampleLocation(elevation, azimuth);
				}
			}
			s_audioMutex.unlock();
			
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
			
			debugTimerBegin("applyHrtf");
			
			HRIRSampleData hrir;
			
			{
				const HRIRSampleData * samples[3];
				float sampleWeights[3];
				
				float elevation = hoverLocation[1];
				float azimuth = hoverLocation[0];
				
				{
					// clamp elevation and azimuth to ensure it maps within the elevation and azimut topology
					
					const float eps = .01f;
					
					const float elevationMin = -90.f + eps;
					const float elevationMax = +90.f - eps;
					
					const float azimuthMin = -180.f + eps;
					const float azimuthMax = +180.f - eps;
					
					elevation = std::max(elevation, elevationMin);
					elevation = std::min(elevation, elevationMax);
					
					azimuth = std::max(azimuth, azimuthMin);
					azimuth = std::min(azimuth, azimuthMax);
				}
				
				if (sampleSet.lookup_3(elevation, azimuth, samples, sampleWeights))
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
			
			AudioBuffer_Real audioBufferL;
			AudioBuffer_Real audioBufferR;
			
		#if BLEND_PREVIOUS_HRTF
			// convolve audio in the frequency domain
			
			const HRTF & oldHrtf = previousHrtf;
			const HRTF & newHrtf = hrtf;
			
			AudioBuffer_Real oldAudioBufferL;
			AudioBuffer_Real oldAudioBufferR;
			
			AudioBuffer_Real newAudioBufferL;
			AudioBuffer_Real newAudioBufferR;
			
			convolveAudio_2(
				audioBuffer,
				oldHrtf.lFilter,
				oldHrtf.rFilter,
				newHrtf.lFilter,
				newHrtf.rFilter,
				oldAudioBufferL.samples,
				oldAudioBufferR.samples,
				newAudioBufferL.samples,
				newAudioBufferR.samples);
			
			// ramp from old to new audio buffer
			
			debugTimerBegin("rampAudioBuffers");
			
			rampAudioBuffers(oldAudioBufferL.samples, newAudioBufferL.samples, AUDIO_BUFFER_SIZE, audioBufferL.samples);
			rampAudioBuffers(oldAudioBufferR.samples, newAudioBufferR.samples, AUDIO_BUFFER_SIZE, audioBufferR.samples);
			
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
		
			debugTimerEnd("applyHrtf");
			
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
					
					std::vector<Vec2> sampleLocations;
					sampleLocations.resize(audioHandler.sounds.size());
					s_audioMutex.lock();
					{
						int index = 0;
						for (auto & sound : audioHandler.sounds)
						{
							sampleLocations[index][0] = sound->binauralizer.sampleLocation.elevation;
							sampleLocations[index][1] = sound->binauralizer.sampleLocation.azimuth;
							index++;
						}
					}
					s_audioMutex.unlock();
					
					hqBegin(HQ_FILLED_CIRCLES);
					{
						for (int i = 0; i < sampleLocations.size(); ++i)
						{
							setColor(colorYellow);
							hqFillCircle(sampleLocations[i][1], sampleLocations[i][0], 1.f);
						}
					}
					hqEnd();
				}
				gxPopMatrix();
				
			#if DO_VIEW3D
				pushSurface(&view3D);
				{
					view3D.clear(200, 200, 200);
				
					projectPerspective3d(90.f, .001f, 10.f);
					viewLookat3d(-2.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, -1.f, 0.f);
					
					gxRotatef(framework.time * .1f, 0, 1, 0);
					
					glPointSize(2.f);
					gxBegin(GL_POINTS);
					{
						for (auto & sample : sampleSet.samples)
						{
							const float elevation = sample->elevation;
							const float azimuth = sample->azimuth;
							
							float x, y, z;
							elevationAndAzimuthToCartesian(elevation, azimuth, x, y, z);
							
							setColor(150, 150, 150);
							gxVertex3f(x, y, z);
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
					
					projectScreen2d();
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
			#endif
			
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
							drawLine(i, 0, i, sy/2 + audioBufferL.samples[i] * sy/2);
						}
						
						setColor(colorGreen);
						for (int i = 0; i < sx; ++i)
						{
							setColorf(0.f, 1.f, .5f);
							drawLine(i, 0, i, sy/2 + audioBufferR.samples[i] * sy/2);
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
					gxTranslatef((GFX_SX - AUDIO_UPDATE_SIZE) / 2, GFX_SY - 50, 0);
					
					float samples0[AUDIO_UPDATE_SIZE];
					float samples1[AUDIO_UPDATE_SIZE];
					float samples[AUDIO_UPDATE_SIZE];
					
					for (int i = 0; i < AUDIO_UPDATE_SIZE; ++i)
					{
						samples0[i] = 0.f;
						samples1[i] = 1.f;
						samples[i] = random(0.f, 1.f);
					}
					
					rampAudioBuffers(samples0, samples1, AUDIO_UPDATE_SIZE, samples);
					
					const int sx = AUDIO_UPDATE_SIZE;
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
						drawText(mouse.x, mouse.y + 60, fontSize, 0, 1, "cell.numTriangles=%d", int(hoverCell->triangleIndices.size()));
					}
					
					gxPopMatrix();
				}
				
				gxPushMatrix();
				{
					gxTranslatef(10, 100, 0);
					setColor(colorBlack);
					drawText(0, 0, 16, +1, +1, "time per tick: %.2fms", audioHandler.msecsPerTick / 1000.0);
					drawText(0, 20, 16, +1, +1, "time per second: %.2fms", audioHandler.msecsPerSecond / 1000.0);
				}
				gxPopMatrix();
				
				popFontMode();
			}
			framework.endDraw();
		} while (!keyboard.wentDown(SDLK_ESCAPE));
		
		pa.shut();
	}
	
	fassert(s_audioMutexSDL != nullptr);
	SDL_DestroyMutex(s_audioMutexSDL);
	s_audioMutexSDL = nullptr;
	
	framework.shutdown();

	return 0;
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
				for (auto cellTriangleIndex : hoverCell->triangleIndices)
				{
					auto cellTriangle = &sampleSet.sampleGrid.triangles[cellTriangleIndex];
					
					if (&triangle == cellTriangle)
						isInHoverCell = true;
				}
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
