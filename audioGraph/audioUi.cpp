/*
	Copyright (C) 2020 Marcel Smit
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

#include "audioGraphManager.h"
#include "audioUi.h"
#include "StringEx.h"
#include "ui.h"

bool doAudioGraphSelect(AudioGraphManager_RTE & audioGraphMgr)
{
	bool result = false;
	
	for (auto & fileItr : audioGraphMgr.files)
	{
		auto & filename = fileItr.first;
		
		if (doButton(filename.c_str()))
		{
			audioGraphMgr.selectFile(filename.c_str());
			
			result = true;
		}
	}
	
	return result;
}

bool doAudioGraphInstanceSelect(AudioGraphManager_RTE & audioGraphMgr, std::string & activeInstanceName)
{
	bool result = true;
	
	for (auto & fileItr : audioGraphMgr.files)
	{
		auto & filename = fileItr.first;
		auto file = fileItr.second;
		
		for (auto instance : file->instanceList)
		{
			std::string name = String::FormatC("%s: %p", filename.c_str(), instance->audioGraph);
			
			if (doButton(name.c_str()))
			{
				if (name != activeInstanceName)
				{
					activeInstanceName = name;
					
					audioGraphMgr.selectInstance(instance);
					
					result = true;
				}
			}
		}
	}
	
	return result;
}

//

#include "audioNodeBase.h"
#include "framework.h"

void drawFilterResponse(const AudioNodeBase * node, const float sx, const float sy)
{
	const int kNumSteps = 256;
	float response[kNumSteps];
	
	if (node->getFilterResponse(response, kNumSteps))
	{
		hqBegin(HQ_FILLED_ROUNDED_RECTS);
		{
			setColorf(0, 0, 0, .8f);
			hqFillRoundedRect(0, 0, sx, sy, 4.f);
		}
		hqEnd();
		
		setColor(colorWhite);
		hqBegin(HQ_LINES);
		{
			for (int i = 0; i < kNumSteps - 1; ++i)
			{
				const float x1 = sx * (i + 0) / kNumSteps;
				const float x2 = sx * (i + 1) / kNumSteps;
				const float y1 = (1.f - response[i + 0]) * sy;
				const float y2 = (1.f - response[i + 1]) * sy;
				
				hqLine(x1, y1, 3.f, x2, y2, 3.f);
			}
		}
		hqEnd();
	}
}
