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

#include "audioGraph.h"
#include "audioNodeTime.h"

AUDIO_ENUM_TYPE(timeMode)
{
	elem("graph");
	elem("audio");
}

AUDIO_NODE_TYPE(AudioNodeTime)
{
	typeName = "time";
	
	in("fine", "bool", "1");
	inEnum("mode", "timeMode");
	in("scale", "float", "1");
	in("offset", "float");
	out("time", "audioValue");
}

static double getAudioTime(const AudioNodeTime::Mode mode)
{
	if (mode == AudioNodeTime::kMode_AudioLocal)
		return g_currentAudioTime;
	else
		return g_currentAudioGraph->time;
}

AudioNodeTime::AudioNodeTime()
	: AudioNodeBase()
	, resultOutput()
{
	resizeSockets(kInput_COUNT, kOutput_COUNT);
	addInput(kInput_FineGrained, kAudioPlugType_Bool);
	addInput(kInput_Mode, kAudioPlugType_Int);
	addInput(kInput_Scale, kAudioPlugType_Float);
	addInput(kInput_Offset, kAudioPlugType_Float);
	addOutput(kOutput_Result, kAudioPlugType_FloatVec, &resultOutput);
}

void AudioNodeTime::tick(const float dt)
{
	const bool fineGrained = getInputBool(kInput_FineGrained, true);
	const Mode mode = (Mode)getInputInt(kInput_Mode, 0);
	const float scale = getInputFloat(kInput_Scale, 1.f);
	const float offset = getInputFloat(kInput_Offset, 0.f);
	
	double time = getAudioTime(mode);
	
	if (fineGrained)
	{
		resultOutput.setVector();
		
		for (int i = 0; i < AUDIO_UPDATE_SIZE; ++i)
		{
			resultOutput.samples[i] = time * scale + offset;
			
			time += 1.0 / SAMPLE_RATE;
		}
	}
	else
	{
		time += 1.0 / SAMPLE_RATE * (AUDIO_UPDATE_SIZE / 2);
		
		resultOutput.setScalar(time * scale + offset);
	}
}
