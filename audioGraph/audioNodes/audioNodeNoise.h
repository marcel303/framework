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

#pragma once

#include "audioNodeBase.h"
#include "Random.h"

struct AudioNodeNoise : AudioNodeBase
{
	enum Type
	{
		kType_Octave,
		kType_White,
		kType_Pink,
		kType_Brown
	};
	
	enum Input
	{
		kInput_Type,
		kInput_Fine,
		kInput_NumOctaves,
		kInput_SampleRate,
		kInput_Scale,
		kInput_Persistence,
		kInput_Min,
		kInput_Max,
		kInput_X,
		kInput_COUNT
	};
	
	enum Output
	{
		kOutput_Result,
		kOutput_COUNT
	};
	
	AudioFloat resultOutput;
	
	RNG::PinkNumber pinkNumber;
	
	double brownValue;
	
	AudioNodeNoise()
		: AudioNodeBase()
		, resultOutput()
		, pinkNumber(1 << 16)
		, brownValue(0.0)
	{
		resizeSockets(kInput_COUNT, kOutput_COUNT);
		addInput(kInput_Type, kAudioPlugType_Int);
		addInput(kInput_Fine, kAudioPlugType_Bool);
		addInput(kInput_NumOctaves, kAudioPlugType_Int);
		addInput(kInput_SampleRate, kAudioPlugType_Int);
		addInput(kInput_Scale, kAudioPlugType_FloatVec);
		addInput(kInput_Persistence, kAudioPlugType_FloatVec);
		addInput(kInput_Min, kAudioPlugType_FloatVec);
		addInput(kInput_Max, kAudioPlugType_FloatVec);
		addInput(kInput_X, kAudioPlugType_FloatVec);
		addOutput(kOutput_Result, kAudioPlugType_FloatVec, &resultOutput);
	}
	
	void drawOctave();
	void drawWhite();
	void drawPink();
	void drawBrown();
	
	virtual void tick(const float dt) override;
};
