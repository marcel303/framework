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

#include "framework.h"
#include "xmm/xmm.h"

extern const int GFX_SX;
extern const int GFX_SY;

struct Recording
{
	int phraseIndex;
	
	std::vector<Vec2> points;
	
	void getMinMax(Vec2 & min, Vec2 & max) const
	{
		min = points[0];
		max = points[0];
		
		for (int i = 1; i < points.size(); ++i)
		{
			min = min.Min(points[i]);
			max = max.Max(points[i]);
		}
	}
};

void testXmm()
{
	{
		// build training set
		
		xmm::TrainingSet trainingSet;
		trainingSet.dimension.set(2);
		trainingSet.column_names = std::vector<std::string> { "x", "y" };
		
		std::vector<float> observation;
		observation.resize(2);
		
		trainingSet.addPhrase(0, "A");
		trainingSet.addPhrase(1, "B");
		
		for (int i = 0; i < 100; ++i)
		{
			observation[0] = i / 100.f;
			observation[1] = i / 100.f;
			
			trainingSet.getPhrase(0)->record(observation);
		}
		
		for (int i = 0; i < 100; ++i)
		{
			observation[0] = i / 100.f;
			observation[1] = i / 200.f;
			
			trainingSet.getPhrase(1)->record(observation);
		}
		
		// start learning
		
		xmm::GMM gmm;
		
		// set parameters
		gmm.configuration.gaussians.set(10);
		gmm.configuration.relative_regularization.set(0.01);
		gmm.configuration.absolute_regularization.set(0.0001);
		gmm.configuration.multithreading = xmm::MultithreadingMode::Sequential;
		
		gmm.train(&trainingSet);
		gmm.reset();
		
		xmm::HierarchicalHMM hhmm;
		hhmm.train(&trainingSet);
		hhmm.reset();
		
		logDebug("GMM: number of models: %d", gmm.size());
		
		// start recognition
		
		gmm.shared_parameters->likelihood_window.set(40);
		hhmm.shared_parameters->likelihood_window.set(40);
		
		for (int i = 0; i < 100; ++i)
		{
			observation[0] = i / 100.f;
			observation[1] = i / 100.f;
			
			hhmm.filter(observation);
			
			logDebug("[%02d] hhmm: A: %.2f, progress: %.2f. B: %.2f, progress: %.2f",
				hhmm.models["A"].results.instant_likelihood,
				hhmm.models["A"].results.progress,
				hhmm.models["B"].results.instant_likelihood,
				hhmm.models["B"].results.progress);
		}
		
		for (int i = 0; i < 100; ++i)
		{
			observation[0] = i / 100.f;
			observation[1] = i / 100.f;
			
			gmm.filter(observation);
			
			logDebug("likeliest: %s. A: %.2f, B: %.2f",
				gmm.results.likeliest.c_str(),
				gmm.results.instant_normalized_likelihoods[0],
				gmm.results.instant_normalized_likelihoods[1]);
		}
		
		const int n = 1;
		
		for (int i = 0; i < 100 * n; ++i)
		{
			observation[0] = i / 100.f / n;
			observation[1] = i / 200.f / n;
			
			gmm.filter(observation);
			
			auto n = gmm.results.instant_normalized_likelihoods;
			
			logDebug("likeliest: %s. A: %.2f, B: %.2f",
				gmm.results.likeliest.c_str(),
				gmm.results.instant_likelihoods[0],
				gmm.results.instant_likelihoods[1]);
		}
		
		logDebug("done!");
	}
	
	xmm::TrainingSet trainingSet;
	trainingSet.dimension.set(2);
	trainingSet.column_names = std::vector<std::string> { "x", "y" };
	
	std::vector<float> observation;
	observation.resize(2);

	xmm::HierarchicalHMM hhmm;
	//hhmm.configuration.hierarchical = false;
	
	std::map<std::string, Recording> recordings;
	
	Recording recording;
	int nextRecordingIndex = 0;
	
	int frameIndex = 0;
	
	bool isRecording = true;
	
	do
	{
		framework.process();
		
		if (keyboard.wentDown(SDLK_r))
		{
			isRecording = !isRecording;
		}
		
		if (mouse.wentDown(BUTTON_LEFT))
		{
			recording = Recording();
			
			hhmm.reset();
		}
		
		if (mouse.isDown(BUTTON_LEFT))
		{
			Vec2 point(mouse.x, mouse.y);
			
			recording.points.push_back(point);
			
			if (isRecording == false)
			{
				observation[0] = point[0];
				observation[1] = point[1];
				
				hhmm.filter(observation);
				
				int index = 0;
				
				for (auto & i : recordings)
				{
					auto & name = i.first;
					
					logDebug("hhmm: %s: certainty: %.2f, progress: %.2f",
						name.c_str(),
						hhmm.results.instant_normalized_likelihoods[index],
						//hhmm.models[name].results.log_likelihood,
						hhmm.models[name].results.progress);
					
					index++;
				}
			}
		}
		
		if (mouse.wentUp(BUTTON_LEFT))
		{
			if (isRecording)
			{
				char name[32];
				sprintf(name, "%d", nextRecordingIndex);
				
				recording.phraseIndex = nextRecordingIndex;
				recordings[name] = recording;
				
				hhmm = xmm::HierarchicalHMM();
				hhmm.shared_parameters->likelihood_window.set(40);
				
				for (auto & i : recordings)
				{
					auto & n = i.first;
					auto & r = i.second;
					
					trainingSet.addPhrase(r.phraseIndex, n);
					auto p = trainingSet.getPhrase(r.phraseIndex).get();
					
					for (int i = 0; i < r.points.size(); ++i)
					{
						observation[0] = r.points[i][0];
						observation[1] = r.points[i][1];
						p->record(observation);
					}
				}
				
				hhmm.train(&trainingSet);
				hhmm.reset();
				
				nextRecordingIndex++;
			}
		}
		
		framework.beginDraw(0, 0, 0, 0);
		{
			auto drawRecording = [frameIndex](const Recording & recording, const Color & outline, const Color & line, const bool useView, const Vec2 & viewMin, const Vec2 & viewMax)
			{
				Vec2 min;
				Vec2 max;
				
				recording.getMinMax(min, max);
				
				if (useView)
				{
					setColor(100, 100, 100);
					drawRectLine(viewMin[0], viewMin[1], viewMax[0], viewMax[1]);
					
					gxPushMatrix();
					
					const float scaleX = (viewMax[0] - viewMin[0]) / (max[0] - min[0]);
					const float scaleY = (viewMax[1] - viewMin[1]) / (max[1] - min[1]);
					const float scale = std::min(scaleX, scaleY);
					
					gxTranslatef(viewMin[0], viewMin[1], 0.f);
					gxScalef(scale, scale, 1.f);
					gxTranslatef(-min[0], -min[1], 0.f);
				}
				
				setColor(outline);
				drawRectLine(min[0], min[1], max[0], max[1]);
				
				setColor(line);
				hqBegin(HQ_LINES);
				{
					for (int i = 0; i < recording.points.size() - 1; ++i)
					{
						const Vec2 & p1 = recording.points[i + 0];
						const Vec2 & p2 = recording.points[i + 1];
						
						hqLine(p1[0], p1[1], 2.5f, p2[0], p2[1], 2.5f);
					}
				}
				hqEnd();
				
				setColor(line);
				hqBegin(HQ_FILLED_CIRCLES);
				{
					for (int i = 0; i < recording.points.size(); ++i)
					{
						const bool isHighlighted = (frameIndex % recording.points.size()) == i;
						
						const Vec2 & p = recording.points[i];
						
						if (isHighlighted)
							setColor(colorWhite);
						
						hqFillCircle(p[0], p[1], 5.f);
						
						if (isHighlighted)
							setColor(line);
					}
				}
				hqEnd();
				
				if (useView)
				{
					gxPopMatrix();
				}
			};
			
			int index = 0;
			
			for (auto & i : recordings)
			{
				const int numX = 5;
				const int viewSx = GFX_SX/numX;
				const int viewSy = viewSx;
				
				const int cellX = index % numX;
				const int cellY = index / numX;
				
				const Vec2 viewMin((cellX + 0) * viewSx, (cellY + 0) * viewSy);
				const Vec2 viewMax((cellX + 1) * viewSx, (cellY + 1) * viewSy);
				
				drawRecording(i.second, colorBlue, colorRed, true, viewMin, viewMax);
				
				index++;
			}
			
			if (isRecording)
			{
				if (recording.points.empty() == false)
				{
					drawRecording(recording, colorGreen, colorWhite, false, Vec2(), Vec2());
				}
			}
			else
			{
				if (recording.points.empty() == false)
				{
					drawRecording(recording, colorGreen, Color(100, 100, 100), false, Vec2(), Vec2());
				}
			}
			
			setFontMSDF("calibri.ttf");
			drawTextMSDF(GFX_SX/2, GFX_SY-20, 18, 0, 0, "%s", isRecording ? "RECORDING" : "RECOGNIZING");
			drawTextMSDF(GFX_SX/2, GFX_SY-80, 18, 0, 0, "%s", isRecording ? "(n/a)" : hhmm.results.likeliest.c_str());
		}
		framework.endDraw();
		
		frameIndex++;
		
	} while (!keyboard.wentDown(SDLK_SPACE));
}
