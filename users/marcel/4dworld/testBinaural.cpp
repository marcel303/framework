#include "binaural.h"
#include "binaural_ircam.h"
#include "binaural_mit.h"
#include "framework.h"

using namespace binaural;

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
		
		do
		{
			framework.process();
			
			//
			
			if (mouse.isDown(BUTTON_LEFT))
			{
				if (mouse.dy > 0.f)
					scale *= 1.f + std::fabsf(mouse.dy) / 100.f;
				else
					scale /= 1.f + std::fabsf(mouse.dy) / 100.f;
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
			
			//
			
			const Vec2 hoverLocation = transform.Invert() * Vec2(mouse.x, mouse.y);
			
			float baryU;
			float baryV;
			
			auto hoverCell = sampleSet.sampleGrid.lookup(hoverLocation[1], hoverLocation[0], baryU, baryV);
			
			//
			
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
			
			//
			
			HRTF hrtf;
			
			hrirToHrtf(hrir.lSamples, hrir.rSamples, hrtf.lFilter, hrtf.rFilter);
			
			//
			
			framework.beginDraw(230, 230, 230, 0);
			{
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
				}
				gxPopMatrix();
				
				//
				
				for (int offset = 1; offset >= 0; --offset)
				{
					gxPushMatrix();
					gxTranslatef(offset, offset, 0);
					
					setFont("calibri.ttf");
					setColor(offset == 0 ? colorWhite : Color(0, 0, 0, 127));
					
					drawText(mouse.x, mouse.y + 20, 14, 0, 1, "azimuth=%.2f, elevation=%.2f", hoverLocation[0], hoverLocation[1]);
					
					if (hoverCell != nullptr)
					{
						drawText(mouse.x, mouse.y + 40, 14, 0, 1, "bary=(%.2f, %.2f, %.2f)", baryU, baryV, 1.f - baryU - baryV);
					}
					
					gxPopMatrix();
				}
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
