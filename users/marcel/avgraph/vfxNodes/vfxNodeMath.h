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

#include "vfxNodeBase.h"

struct VfxNodeMath : VfxNodeBase
{
	enum Input
	{
		kInput_Type,
		kInput_A,
		kInput_B,
		kInput_COUNT
	};
	
	enum Output
	{
		kOutput_R,
		kOutput_COUNT
	};
	
	enum Type
	{
		kType_Unknown,
		kType_Add,
		kType_Sub,
		kType_Mul,
		kType_Sin,
		kType_Cos,
		kType_Abs,
		kType_Min,
		kType_Max,
		kType_Sat,
		kType_Neg,
		kType_Sqrt,
		kType_Pow,
		kType_Exp,
		kType_Mod,
		kType_Fract,
		kType_Floor,
		kType_Ceil,
		kType_Round,
		kType_Sign,
		kType_Hypot,
		kType_Pitch,
		kType_Semitone
	};
	
	float result;
	
	VfxNodeMath();
	
	virtual void tick(const float dt) override;
};

struct VfxNodeMathBase : VfxNodeBase
{
	enum Input
	{
		kInput_A,
		kInput_B,
		kInput_COUNT
	};
	
	enum Output
	{
		kOutput_R,
		kOutput_COUNT
	};
	
	VfxNodeMath::Type type;
	float result;
	
	VfxNodeMathBase(VfxNodeMath::Type type);
	
	virtual void tick(const float dt) override;
};
