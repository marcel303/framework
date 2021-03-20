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
#include "audioNodeMemf.h"

// todo : add scope

AUDIO_NODE_TYPE(AudioNodeMemf)
{
	typeName = "memf";
	
	in("name", "string");
	out("value1", "audioValue");
	out("value2", "audioValue");
	out("value3", "audioValue");
	out("value4", "audioValue");
}

AudioNodeMemf::AudioNodeMemf()
	: AudioNodeBase()
	, currentName()
	, valueOutput()
{
	resizeSockets(kInput_COUNT, kOutput_COUNT);
	addInput(kInput_Name, kAudioPlugType_String);
	addOutput(kOutput_Value1, kAudioPlugType_FloatVec, &valueOutput[0]);
	addOutput(kOutput_Value2, kAudioPlugType_FloatVec, &valueOutput[1]);
	addOutput(kOutput_Value3, kAudioPlugType_FloatVec, &valueOutput[2]);
	addOutput(kOutput_Value4, kAudioPlugType_FloatVec, &valueOutput[3]);
}

void AudioNodeMemf::tick(const float dt)
{
	const char * name = getInputString(kInput_Name, nullptr);

	if (isPassthrough || name == nullptr)
	{
		currentName.clear();
		
		valueOutput[0].setScalar(0.f);
		valueOutput[1].setScalar(0.f);
		valueOutput[2].setScalar(0.f);
		valueOutput[3].setScalar(0.f);
	}
	else
	{
		if (name != currentName)
		{
			currentName = name;
			
			g_currentAudioGraph->registerMemf(name, 0.f, 0.f, 0.f, 0.f);
		}
		
		const AudioGraph::Memf memf = g_currentAudioGraph->getMemf(name, false);

		valueOutput[0].setScalar(memf.value1_audioThread);
		valueOutput[1].setScalar(memf.value2_audioThread);
		valueOutput[2].setScalar(memf.value3_audioThread);
		valueOutput[3].setScalar(memf.value4_audioThread);
	}
}

void AudioNodeMemf::init(const GraphNode & node)
{
	const char * name = getInputString(kInput_Name, nullptr);

	if (isPassthrough || name == nullptr)
		return;
	
	currentName = name;
	
	g_currentAudioGraph->registerMemf(name, 0.f, 0.f, 0.f, 0.f);
}
