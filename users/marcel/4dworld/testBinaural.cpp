#include "binaural.h"
#include "binaural_ircam.h"
#include "binaural_mit.h"
#include "framework.h"

using namespace binaural;

extern const int GFX_SX;
extern const int GFX_SY;

static void drawHrirSampleGrid(HRIRSampleSet & dataSet, HRIRSampleGrid & grid);

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
			
			framework.beginDraw(0, 0, 0, 0);
			{
				gxPushMatrix();
				{
					const float scale = 1.f + mouse.y * 5.f / GFX_SY;
					
					gxTranslatef(GFX_SX/2, GFX_SY/2, 0);
					gxScalef(+scale, -scale, 1.f);
					
					drawHrirSampleGrid(sampleSet, sampleSet.sampleGrid);
				}
				gxPopMatrix();
			}
			framework.endDraw();
		} while (!keyboard.wentDown(SDLK_SPACE));
	}
}

static void drawHrirSampleGrid(HRIRSampleSet & dataSet, HRIRSampleGrid & grid)
{
	gxBegin(GL_TRIANGLES);
	{
		int index = 0;
		
		for (auto & cell : grid.cells)
		{
			auto & p1 = cell.vertex[0].location;
			auto & p2 = cell.vertex[1].location;
			auto & p3 = cell.vertex[2].location;
			
			setColorf(
				p1.azimuth   / 360.f,
				p1.elevation / 180.f,
				(index % 32) / 31.f);
			
			gxVertex2f(p2.azimuth, p2.elevation);
			gxVertex2f(p1.azimuth, p1.elevation);
			gxVertex2f(p3.azimuth, p3.elevation);
			
			index++;
		}
	}
	gxEnd();
	
	gxBegin(GL_POINTS);
	{
		for (auto & sample : dataSet.samples)
		{
			setColor(colorWhite);
			gxVertex2f(sample->azimuth, sample->elevation);
		}
	}
	gxEnd();
}
