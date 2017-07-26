#include "binaural.h"
#include "binaural_ircam.h"
#include "binaural_mit.h"
#include "framework.h"

using namespace binaural;

#define AUDIO_UPDATE_SIZE (AUDIO_BUFFER_SIZE/2)

#define BLEND_PREVIOUS_HRTF 1

extern const int GFX_SX;
extern const int GFX_SY;

static void drawHrirSampleGrid(const HRIRSampleSet & dataSet, const Vec2 & hoverLocation, const HRIRSampleGrid::Cell * hoverCell);

void testBinaural()
{
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
		
		AudioBuffer overlapBuffer;
		memset(&overlapBuffer, 0, sizeof(overlapBuffer));
		
		float oscillatorPhase = 0.f;
		
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
			
			float baryU;
			float baryV;
			
			auto hoverCell = sampleSet.sampleGrid.lookup(hoverLocation[1], hoverLocation[0], baryU, baryV);
			
			// generate audio signal
			
			memcpy(overlapBuffer.real, overlapBuffer.real + AUDIO_UPDATE_SIZE, (AUDIO_BUFFER_SIZE - AUDIO_UPDATE_SIZE) * sizeof(float));
			
			float * __restrict samples = overlapBuffer.real + AUDIO_BUFFER_SIZE - AUDIO_UPDATE_SIZE;
			
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
			reverseSampleIndices(overlapBuffer.real, audioBuffer.real);
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
			
			rampAudioBuffers(oldAudioBufferL.real, newAudioBufferL.real, audioBufferL.real);
			rampAudioBuffers(oldAudioBufferR.real, newAudioBufferR.real, audioBufferR.real);
			
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
					
					drawHrirSampleGrid(sampleSet, hoverLocation, hoverCell);
				}
				gxPopMatrix();
				
				//
				
				gxPushMatrix();
				{
					const int sx = HRIR_BUFFER_SIZE;
					const int sy = 100;
					
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
							drawLine(i, 0, i, sy/2 + overlapBuffer.real[i] * sy/2);
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
					gxTranslatef(0, GFX_SY - 100, 0);
					
					const int sx = HRTF_BUFFER_SIZE;
					const int sy = 100;
					
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
					gxTranslatef(GFX_SX - HRIR_BUFFER_SIZE, GFX_SY - 100, 0);
					
					const int sx = HRTF_BUFFER_SIZE;
					const int sy = 100;
					
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
				
				//
				
				for (int offset = 1; offset >= 0; --offset)
				{
					gxPushMatrix();
					gxTranslatef(offset, offset, 0);
					
					setColor(offset == 0 ? colorWhite : Color(0, 0, 0, 127));
					
					drawText(mouse.x, mouse.y + 20, fontSize, 0, 1, "azimuth=%.2f, elevation=%.2f", hoverLocation[0], hoverLocation[1]);
					
					if (hoverCell != nullptr)
					{
						drawText(mouse.x, mouse.y + 40, fontSize, 0, 1, "bary=(%.2f, %.2f, %.2f)", baryU, baryV, 1.f - baryU - baryV);
					}
					
					gxPopMatrix();
				}
				
				popFontMode();
			}
			framework.endDraw();
		} while (!keyboard.wentDown(SDLK_SPACE));
	}
}

static void drawHrirSampleGrid(const HRIRSampleSet & sampleSet, const Vec2 & hoverLocation, const HRIRSampleGrid::Cell * hoverCell)
{
	hqBegin(HQ_FILLED_TRIANGLES);
	{
		int index = 0;
		
		for (auto & cell : sampleSet.sampleGrid.cells)
		{
			auto & p1 = cell.vertex[0].location;
			auto & p2 = cell.vertex[1].location;
			auto & p3 = cell.vertex[2].location;
			
			if (&cell == hoverCell)
			{
				setColor(colorYellow);
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
