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
	, currentScope(kScope_None)
	, currentType(kType_None)
	, currentMin(0.f)
	, currentMax(0.f)
	, currentSmoothness(0.f)
	, currentDefaultX(0.f)
	, currentDefaultY(0.f)
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
		if (currentScope == kScope_Shared)
		{
			g_currentAudioGraph->globals->unregisterControlValue(currentName.c_str());
			isRegistered = false;
		}
		else if (currentScope == kScope_PerInstance)
		{
			g_currentAudioGraph->unregisterControlValue(currentName.c_str());
			isRegistered = false;
		}
		else
		{
			Assert(false);
		}
	}
}

void AudioNodeControlValue::updateControlValueRegistration()
{
	const char * name = getInputString(kInput_Name, "");
	const Type type = (Type)getInputInt(kInput_Type, 0);
	const Scope scope = (Scope)getInputInt(kInput_Scope, 0);
	const float min = getInputAudioFloat(kInput_Min, &AudioFloat::Zero)->getMean();
	const float max = getInputAudioFloat(kInput_Max, &AudioFloat::One)->getMean();
	const float smoothness = getInputAudioFloat(kInput_Smoothness, &AudioFloat::Zero)->getMean();
	const float defaultX = getInputAudioFloat(kInput_DefaultX, &AudioFloat::Zero)->getMean();
	const float defaultY = getInputAudioFloat(kInput_DefaultY, &AudioFloat::Zero)->getMean();
	
	bool changed = false;
	
	if (name != currentName ||
		scope != currentScope ||
		type != currentType ||
		min != currentMin ||
		max != currentMax ||
		smoothness != currentSmoothness ||
		defaultX != currentDefaultX ||
		defaultY != currentDefaultY)
	{
		changed = true;
	}
	
	if (changed)
	{
		// unregister
		
		if (currentScope == kScope_None)
		{
			//
		}
		else if (currentScope == kScope_Shared)
		{
			if (isRegistered)
			{
				g_currentAudioGraph->globals->unregisterControlValue(currentName.c_str());
				isRegistered = false;
			}
		}
		else if (currentScope == kScope_PerInstance)
		{
			if (isRegistered)
			{
				g_currentAudioGraph->unregisterControlValue(currentName.c_str());
				isRegistered = false;
			}
		}
		else
		{
			Assert(false);
		}
		
		//
		
		currentName = name;
		currentScope = scope;
		currentType = type;
		currentMin = min;
		currentMax = max;
		currentSmoothness = smoothness;
		currentDefaultX = defaultX;
		currentDefaultY = defaultY;
		
		AudioControlValue::Type _type =
			type == kType_Vector1D ? AudioControlValue::kType_Vector1d :
			type == kType_Vector2D ? AudioControlValue::kType_Vector2d :
			type == kType_Random1D ? AudioControlValue::kType_Random1d :
			type == kType_Random2D ? AudioControlValue::kType_Random2d :
			AudioControlValue::kType_Vector1d;
		
		if (!currentName.empty())
		{
			if (scope == kScope_Shared)
			{
				g_currentAudioGraph->globals->registerControlValue(
					_type,
					currentName.c_str(),
					min,
					max,
					smoothness,
					defaultX,
					defaultY);
				
				isRegistered = true;
			}
			else if (scope == kScope_PerInstance)
			{
				g_currentAudioGraph->registerControlValue(
					_type,
					currentName.c_str(),
					min,
					max,
					smoothness,
					defaultX,
					defaultY);
				
				isRegistered = true;
			}
			else
			{
				Assert(false);
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
			AudioGraphGlobals::Memf memf = g_currentAudioGraph->globals->getMemf(name);

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

void AudioNodeControlValue::init(const GraphNode & node)
{
	updateControlValueRegistration();
}

void AudioNodeControlValue::tick(const float dt)
{
	updateControlValueRegistration();
}
