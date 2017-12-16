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

struct AudioNodeControlValue : AudioNodeBase
{
	enum Type
	{
		kType_None = -1,
		kType_Vector1D,
		kType_Vector2D,
		kType_Random1D,
		kType_Random2D
	};

	enum Scope
	{
		kScope_None = -1,
		kScope_Shared,
		kScope_PerInstance
	};

	enum Input
	{
		kInput_Name,
		kInput_Type,
		kInput_Scope,
		kInput_Min,
		kInput_Max,
		kInput_Smoothness,
		kInput_DefaultX,
		kInput_DefaultY,
		kInput_COUNT
	};
	
	enum Output
	{
		kOutput_ValueX,
		kOutput_ValueY,
		kOutput_COUNT
	};
	
	std::string currentName;
	Scope currentScope;
	Type currentType;
	float currentMin;
	float currentMax;
	float currentSmoothness;
	float currentDefaultX;
	float currentDefaultY;
	bool isRegistered;
	
	AudioFloat valueOutput[2];

	AudioNodeControlValue();
	virtual ~AudioNodeControlValue() override;
	
	void updateControlValueRegistration();
	
	virtual void init(const GraphNode & node) override;
	virtual void tick(const float dt) override;
};
