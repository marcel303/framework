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
	
	Type type;
	float result;
	
	VfxNodeMath(Type _type)
		: VfxNodeBase()
		, type(kType_Unknown)
		, result(0.f)
	{
		type = _type;
		
		resizeSockets(kInput_COUNT, kOutput_COUNT);
		addInput(kInput_A, kVfxPlugType_Float);
		addInput(kInput_B, kVfxPlugType_Float);
		addOutput(kOutput_R, kVfxPlugType_Float, &result);
	}
	
	virtual void tick(const float dt) override
	{
		const float a = getInputFloat(kInput_A, 0.f);
		const float b = getInputFloat(kInput_B, 0.f);
		
		float r;
		
		switch (type)
		{
		case kType_Unknown:
			r = 0.f;
			break;
			
		case kType_Add:
			r = a + b;
			break;
		
		case kType_Sub:
			r = a - b;
			break;
			
		case kType_Mul:
			r = a * b;
			break;
			
		case kType_Sin:
			if (isPassthrough)
				r = a;
			else
				r = std::sin(a);
			break;
			
		case kType_Cos:
			if (isPassthrough)
				r = a;
			else
				r = std::cos(a);
			break;
			
		case kType_Abs:
			if (isPassthrough)
				r = a;
			else
				r = std::abs(a);
			break;
			
		case kType_Min:
			r = std::min(a, b);
			break;
			
		case kType_Max:
			r = std::max(a, b);
			break;
			
		case kType_Sat:
			if (isPassthrough)
				r = a;
			else
				r = std::max(0.f, std::min(1.f, a));
			break;
			
		case kType_Neg:
			if (isPassthrough)
				r = a;
			else
				r = -a;
			break;
			
		case kType_Sqrt:
			if (isPassthrough)
				r = a;
			else
				r = std::sqrt(a);
			break;
			
		case kType_Pow:
			if (isPassthrough)
				r = a;
			else
				r = std::pow(a, b);
			break;
			
		case kType_Exp:
			if (isPassthrough)
				r = a;
			else
				r = std::exp(a);
			break;
			
		case kType_Mod:
			if (isPassthrough)
				r = a;
			else
				r = std::fmod(a, b);
			break;
			
		case kType_Fract:
			if (isPassthrough)
				r = a;
			else if (a >= 0.f)
				r = a - std::floor(a);
			else
				r = a - std::ceil(a);
			break;
			
		case kType_Floor:
			if (isPassthrough)
				r = a;
			else
				r = std::floor(a);
			break;
			
		case kType_Ceil:
			if (isPassthrough)
				r = a;
			else
				r = std::ceil(a);
			break;
			
		case kType_Round:
			if (isPassthrough)
				r = a;
			else
				r = std::round(a);
			break;
			
		case kType_Sign:
			if (isPassthrough)
				r = a;
			else
				r = a < 0.f ? -1.f : +1.f;
			break;
			
		case kType_Hypot:
			r = std::hypot(a, b);
			break;
		
		case kType_Pitch:
			if (isPassthrough)
				r = a;
			else
				r = a * std::powf(2.f, b);
			break;
			
		case kType_Semitone:
			if (isPassthrough)
				r = a;
			else
				r = a * std::powf(2.f, b / 12.f);
			break;
		}
		
		result = r;
	}
};

#define DefineMathNode(name, type) \
	struct name : VfxNodeMath \
	{ \
		name() \
			: VfxNodeMath(type) \
		{ \
		} \
	};

DefineMathNode(VfxNodeMathAdd, kType_Add);
DefineMathNode(VfxNodeMathSub, kType_Sub);
DefineMathNode(VfxNodeMathMul, kType_Mul);
DefineMathNode(VfxNodeMathSin, kType_Sin);
DefineMathNode(VfxNodeMathCos, kType_Cos);
DefineMathNode(VfxNodeMathAbs, kType_Abs);
DefineMathNode(VfxNodeMathMin, kType_Min);
DefineMathNode(VfxNodeMathMax, kType_Max);
DefineMathNode(VfxNodeMathSat, kType_Sat);
DefineMathNode(VfxNodeMathNeg, kType_Neg);
DefineMathNode(VfxNodeMathSqrt, kType_Sqrt);
DefineMathNode(VfxNodeMathPow, kType_Pow);
DefineMathNode(VfxNodeMathExp, kType_Exp);
DefineMathNode(VfxNodeMathMod, kType_Mod);
DefineMathNode(VfxNodeMathFract, kType_Fract);
DefineMathNode(VfxNodeMathFloor, kType_Floor);
DefineMathNode(VfxNodeMathCeil, kType_Ceil);
DefineMathNode(VfxNodeMathRound, kType_Round);
DefineMathNode(VfxNodeMathSign, kType_Sign);
DefineMathNode(VfxNodeMathHypot, kType_Hypot);
DefineMathNode(VfxNodeMathPitch, kType_Pitch);
DefineMathNode(VfxNodeMathSemitone, kType_Semitone);

#undef DefineMathNode
