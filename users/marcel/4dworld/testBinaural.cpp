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
		
		do
		{
			framework.process();
			
			//
			
			//const float scale = 1.f + mouse.y * 5.f / GFX_SY;
			const float scale = 2.f;
			
			Mat4x4 transform;
			transform = Mat4x4(true).Translate(GFX_SX/2, GFX_SY/2, 0).Scale(+scale, -scale, 1.f);
			
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
			
			framework.beginDraw(0, 0, 0, 0);
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
					gxTranslatef(HRIR_BUFFER_SIZE + 16, 0, 0);
					
					const int sx = HRTF_BUFFER_SIZE;
					const int sy = 100;
					
					setColor(colorBlack);
					drawRect(0, 0, sx, sy);
					pushBlend(BLEND_ADD);
					for (int i = 0; i < sx; ++i)
					{
						const float power = std::hypotf(hrtf.lFilter.real[i], hrtf.lFilter.imag[i]);
						setColorf(1.f, 0.f, 0.f, power);
						drawLine(i, 0, i, sy);
					}
					popBlend();
				}
				gxPopMatrix();
				
				gxPushMatrix();
				{
					gxTranslatef(HRIR_BUFFER_SIZE + 16, 110, 0);
					
					const int sx = HRTF_BUFFER_SIZE;
					const int sy = 100;
					
					setColor(colorBlack);
					drawRect(0, 0, sx, sy);
					pushBlend(BLEND_ADD);
					for (int i = 0; i < sx; ++i)
					{
						const float power = std::hypotf(hrtf.rFilter.real[i], hrtf.rFilter.imag[i]);
						setColorf(0.f, 1.f, 0.f, power);
						drawLine(i, 0, i, sy);
					}
					popBlend();
				}
				gxPopMatrix();
				
				//
				
				setFont("calibri.ttf");
	
				if (hoverCell != nullptr)
				{
					setColor(63, 255, 0, 191);
					drawText(mouse.x, mouse.y + 20, 14, 0, 1, "%.2f, %.2f", baryU, baryV);
				}
			}
			framework.endDraw();
		} while (!keyboard.wentDown(SDLK_SPACE));
	}
}

static void drawHrirSampleGrid(const HRIRSampleSet & sampleSet, const Vec2 & hoverLocation, const HRIRSampleGrid::Cell * hoverCell)
{
	gxBegin(GL_TRIANGLES);
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
			
			gxVertex2f(p2.azimuth, p2.elevation);
			gxVertex2f(p1.azimuth, p1.elevation);
			gxVertex2f(p3.azimuth, p3.elevation);
			
			index++;
		}
	}
	gxEnd();
	
	gxBegin(GL_POINTS);
	{
		for (auto & sample : sampleSet.samples)
		{
			setColor(colorWhite);
			gxVertex2f(sample->azimuth, sample->elevation);
		}
	}
	gxEnd();
	
	hqBegin(HQ_FILLED_CIRCLES);
	{
		setColor(colorWhite);
		hqFillCircle(hoverLocation[0], hoverLocation[1], 1.f);
	}
	hqEnd();
}
