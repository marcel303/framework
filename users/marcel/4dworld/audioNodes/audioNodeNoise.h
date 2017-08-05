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

#pragma once

#include "audioNodeBase.h"
#include <stdlib.h>

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
	
	// Voss' pink number object to be used for pink noise generation
	
	struct PinkNumber
	{
		int maxKey;
		int key; 
		int whiteValues[5];
		int range;
		
		PinkNumber(const int _range = 128)
		{
			maxKey = 0x1f; // five bits set
			
			range = _range;
			key = 0;
			
			for (int i = 0; i < 5; ++i)
			{
				whiteValues[i] = rand() % (range/5);
			}
		}
		
		int next()
		{ 
			const int lastKey = key;

			key++;
			
			if (key > maxKey)
				key = 0;
			
			// exclusive-or previous value with current value. this gives a list of bits that have changed
			
			int diff = lastKey ^ key;
			
			int sum = 0;
			
			for (int i = 0; i < 5; ++i)
			{ 
				// if bit changed get new random number for corresponding whiteValue
				
				if (diff & (1 << i))
				{
					whiteValues[i] = rand() % (range/5);
				}
				
				sum += whiteValues[i];
			}
			
			return sum; 
		}
	};
	
	AudioFloat resultOutput;
	
	PinkNumber pinkNumber;
	
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
