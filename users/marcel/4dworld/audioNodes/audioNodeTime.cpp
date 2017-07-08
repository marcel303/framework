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

#include "audioNodeTime.h"

AUDIO_NODE_TYPE(time, AudioNodeTime)
{
	typeName = "time";
	
	in("fine", "bool", "1");
	in("scale", "float", "1");
	in("offset", "float");
	out("time", "audioValue");
}

AudioNodeTime::AudioNodeTime()
	: AudioNodeBase()
	, time(0.0)
	, resultOutput()
{
	resizeSockets(kInput_COUNT, kOutput_COUNT);
	addInput(kInput_FineGrained, kAudioPlugType_Bool);
	addInput(kInput_Scale, kAudioPlugType_Float);
	addInput(kInput_Offset, kAudioPlugType_Float);
	addOutput(kOutput_Result, kAudioPlugType_FloatVec, &resultOutput);
}

void AudioNodeTime::draw()
{
	const bool fineGrained = getInputBool(kInput_FineGrained, true);
	const float scale = getInputFloat(kInput_Scale, 1.f);
	const float offset = getInputFloat(kInput_Offset, 0.f);
	
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
		resultOutput.setScalar(time * scale + offset);
		
		time += AUDIO_UPDATE_SIZE / double(SAMPLE_RATE);
	}
}
