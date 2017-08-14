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
#include "audioGraphManager.h"
#include "audioNodeControlValue.h"

AUDIO_ENUM_TYPE(controlValueType)
{
	elem("vector.1d");
	elem("vector.2d");
	elem("random.1d");
	elem("random.2d");
}

AUDIO_ENUM_TYPE(controlValueScope)
{
	elem("shared");
	elem("perInstance");
}

AUDIO_NODE_TYPE(controlValue, AudioNodeControlValue)
{
	typeName = "in.value";
	
	in("name", "string");
	inEnum("type", "controlValueType");
	inEnum("scope", "controlValueScope");
	in("min", "audioValue", "0");
	in("max", "audioValue", "1");
	in("smoothness", "audioValue", "0");
	in("defaultX", "audioValue", "0");
	in("defaultY", "audioValue", "0");
	out("valueX", "audioValue");
	out("valueY", "audioValue");
}

AudioNodeControlValue::AudioNodeControlValue()
	: AudioNodeBase()
	, currentName()
	, isRegistered(false)
	, valueOutput()
{
	resizeSockets(kInput_COUNT, kOutput_COUNT);
	addInput(kInput_Name, kAudioPlugType_String);
	addInput(kInput_Type, kAudioPlugType_Int);
	addInput(kInput_Scope, kAudioPlugType_Int);
	addInput(kInput_Min, kAudioPlugType_FloatVec);
	addInput(kInput_Max, kAudioPlugType_FloatVec);
	addInput(kInput_Smoothness, kAudioPlugType_FloatVec);
	addInput(kInput_DefaultX, kAudioPlugType_FloatVec);
	addInput(kInput_DefaultY, kAudioPlugType_FloatVec);
	addOutput(kOutput_ValueX, kAudioPlugType_FloatVec, &valueOutput[0]);
	addOutput(kOutput_ValueY, kAudioPlugType_FloatVec, &valueOutput[1]);
}

AudioNodeControlValue::~AudioNodeControlValue()
{
	if (isRegistered)
	{
		g_audioGraphMgr->unregisterControlValue(currentName.c_str());
		isRegistered = false;
	}
}

void AudioNodeControlValue::tick(const float dt)
{
	const char * name = getInputString(kInput_Name, "");
	const Type _type = (Type)getInputInt(kInput_Type, 0);
	const Scope scope = (Scope)getInputInt(kInput_Scope, 0);
	const float min = getInputAudioFloat(kInput_Min, &AudioFloat::Zero)->getMean();
	const float max = getInputAudioFloat(kInput_Max, &AudioFloat::One)->getMean();
	const float smoothness = getInputAudioFloat(kInput_Smoothness, &AudioFloat::Zero)->getMean();
	const float defaultX = getInputAudioFloat(kInput_DefaultX, &AudioFloat::Zero)->getMean();
	const float defaultY = getInputAudioFloat(kInput_DefaultY, &AudioFloat::Zero)->getMean();
	
	AudioControlValue::Type type =
		_type == kType_Vector1D ? AudioControlValue::kType_Vector1d :
		_type == kType_Vector2D ? AudioControlValue::kType_Vector2d :
		_type == kType_Random1D ? AudioControlValue::kType_Random1d :
		_type == kType_Random2D ? AudioControlValue::kType_Random2d :
		AudioControlValue::kType_Vector1d;

	if (scope == kScope_Shared)
	{
		bool changed = false;
		
		if (name != currentName)
		{
			changed = true;
		}
		else
		{
			AudioControlValue controlValue;
			
			if (g_audioGraphMgr->findControlValue(name, controlValue) == false)
			{
				changed = true;
			}
			else
			{
				const float min = getInputAudioFloat(kInput_Min, &AudioFloat::Zero)->getMean();
				const float max = getInputAudioFloat(kInput_Max, &AudioFloat::One)->getMean();
				const float smoothness = getInputAudioFloat(kInput_Smoothness, &AudioFloat::Zero)->getMean();
				const float defaultX = getInputAudioFloat(kInput_DefaultX, &AudioFloat::Zero)->getMean();
				const float defaultY = getInputAudioFloat(kInput_DefaultY, &AudioFloat::Zero)->getMean();
				
				if (type != controlValue.type ||
					min != controlValue.min ||
					max != controlValue.max ||
					smoothness != controlValue.smoothness ||
					defaultX != controlValue.defaultX ||
					defaultY != controlValue.defaultY)
				{
					changed = true;
				}
			}
		}
		
		if (changed)
		{
			if (isRegistered)
			{
				g_audioGraphMgr->unregisterControlValue(currentName.c_str());
				isRegistered = false;
			}
			
			currentName = name;
			
			if (!currentName.empty())
			{
				g_audioGraphMgr->registerControlValue(type, currentName.c_str(), min, max, smoothness, defaultX, defaultY);
				isRegistered = true;
			}
		}
	}
	
	if (isPassthrough)
	{
		valueOutput[0].setScalar(0.f);
		valueOutput[1].setScalar(0.f);
	}
	else
	{
		if (scope == kScope_Shared)
		{
			AudioGraphManager::Memf memf = g_audioGraphMgr->getMemf(name);

			valueOutput[0].setScalar(memf.value1);
			valueOutput[1].setScalar(memf.value2);
		}
		else if (scope == kScope_PerInstance)
		{
			AudioGraph::Memf memf = g_currentAudioGraph->getMemf(name);

			valueOutput[0].setScalar(memf.value1);
			valueOutput[1].setScalar(memf.value2);
		}
		else
		{
			Assert(false);
			
			valueOutput[0].setScalar(0.f);
			valueOutput[1].setScalar(0.f);
		}
	}
}
