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

#include "vfxNodeMath.h"
#include <algorithm>
#include <cmath>

static float evalMathOp(const float a, const float b, const VfxNodeMath::Type type, const bool isPassthrough)
{
	float r = 0.f;
	
	switch (type)
	{
	case VfxNodeMath::kType_Unknown:
		r = 0.f;
		break;
		
	case VfxNodeMath::kType_Add:
		r = a + b;
		break;
	
	case VfxNodeMath::kType_Sub:
		r = a - b;
		break;
		
	case VfxNodeMath::kType_Mul:
		r = a * b;
		break;
		
	case VfxNodeMath::kType_Sin:
		if (isPassthrough)
			r = a;
		else
			r = std::sin(a);
		break;
		
	case VfxNodeMath::kType_Cos:
		if (isPassthrough)
			r = a;
		else
			r = std::cos(a);
		break;
		
	case VfxNodeMath::kType_Abs:
		if (isPassthrough)
			r = a;
		else
			r = std::abs(a);
		break;
		
	case VfxNodeMath::kType_Min:
		r = std::min(a, b);
		break;
		
	case VfxNodeMath::kType_Max:
		r = std::max(a, b);
		break;
		
	case VfxNodeMath::kType_Sat:
		if (isPassthrough)
			r = a;
		else
			r = std::max(0.f, std::min(1.f, a));
		break;
		
	case VfxNodeMath::kType_Neg:
		if (isPassthrough)
			r = a;
		else
			r = -a;
		break;
		
	case VfxNodeMath::kType_Sqrt:
		if (isPassthrough)
			r = a;
		else
			r = std::sqrt(a);
		break;
		
	case VfxNodeMath::kType_Pow:
		if (isPassthrough)
			r = a;
		else
			r = std::pow(a, b);
		break;
		
	case VfxNodeMath::kType_Exp:
		if (isPassthrough)
			r = a;
		else
			r = std::exp(a);
		break;
		
	case VfxNodeMath::kType_Mod:
		if (isPassthrough)
			r = a;
		else
			r = std::fmod(a, b);
		break;
		
	case VfxNodeMath::kType_Fract:
		if (isPassthrough)
			r = a;
		else if (a >= 0.f)
			r = a - std::floor(a);
		else
			r = a - std::ceil(a);
		break;
		
	case VfxNodeMath::kType_Floor:
		if (isPassthrough)
			r = a;
		else
			r = std::floor(a);
		break;
		
	case VfxNodeMath::kType_Ceil:
		if (isPassthrough)
			r = a;
		else
			r = std::ceil(a);
		break;
		
	case VfxNodeMath::kType_Round:
		if (isPassthrough)
			r = a;
		else
			r = std::round(a);
		break;
		
	case VfxNodeMath::kType_Sign:
		if (isPassthrough)
			r = a;
		else
			r = a < 0.f ? -1.f : +1.f;
		break;
		
	case VfxNodeMath::kType_Hypot:
		r = std::hypot(a, b);
		break;
	
	case VfxNodeMath::kType_Pitch:
		if (isPassthrough)
			r = a;
		else
			r = a * std::powf(2.f, b);
		break;
		
	case VfxNodeMath::kType_Semitone:
		if (isPassthrough)
			r = a;
		else
			r = a * std::powf(2.f, b / 12.f);
		break;
	}
	
	return r;
};

//

VFX_ENUM_TYPE(mathType)
{
	elem("add", 1);
	elem("sub");
	elem("mul");
	elem("sin");
	elem("cos");
	elem("abs");
	elem("min");
	elem("max");
	elem("sat");
	elem("neg");
	elem("sqrt");
	elem("pow");
	elem("exp");
	elem("mod");
	elem("fract");
	elem("floor");
	elem("ceil");
	elem("round");
	elem("sign");
	elem("hypot");
	elem("pitch");
	elem("semitone");
}

VFX_NODE_TYPE(math, VfxNodeMath)
{
	typeName = "math";
	
	inEnum("type", "mathType", "1");
	in("a", "float");
	in("b", "float");
	out("result", "float");
}

VfxNodeMath::VfxNodeMath()
	: VfxNodeBase()
	, result(0.f)
{
	resizeSockets(kInput_COUNT, kOutput_COUNT);
	addInput(kInput_Type, kVfxPlugType_Int);
	addInput(kInput_A, kVfxPlugType_Float);
	addInput(kInput_B, kVfxPlugType_Float);
	addOutput(kOutput_R, kVfxPlugType_Float, &result);
}

void VfxNodeMath::tick(const float dt)
{
	const Type type = (Type)getInputInt(kInput_Type, kType_Add);
	const float a = getInputFloat(kInput_A, 0.f);
	const float b = getInputFloat(kInput_B, 0.f);
	
	result = evalMathOp(a, b, type, isPassthrough);
}

//

VfxNodeMathBase::VfxNodeMathBase(VfxNodeMath::Type _type)
	: VfxNodeBase()
	, type(VfxNodeMath::kType_Unknown)
	, result(0.f)
{
	type = _type;
	
	resizeSockets(kInput_COUNT, kOutput_COUNT);
	addInput(kInput_A, kVfxPlugType_Float);
	addInput(kInput_B, kVfxPlugType_Float);
	addOutput(kOutput_R, kVfxPlugType_Float, &result);
}

void VfxNodeMathBase::tick(const float dt)
{
	const float a = getInputFloat(kInput_A, 0.f);
	const float b = getInputFloat(kInput_B, 0.f);
	
	result = evalMathOp(a, b, type, isPassthrough);
}

//

#define DefineMathNode(name, type, _typeName) \
struct name : VfxNodeMathBase \
{ \
	name() \
		: VfxNodeMathBase(VfxNodeMath::type) \
	{ \
	} \
}; \
VFX_NODE_TYPE(name, name) \
{ \
	typeName = _typeName; \
	in("a", "float"); \
	in("b", "float"); \
	out("result", "float"); \
}

DefineMathNode(VfxNodeMathAdd, kType_Add, "math.add");
DefineMathNode(VfxNodeMathSub, kType_Sub, "math.sub");
DefineMathNode(VfxNodeMathMul, kType_Mul, "math.mul");
DefineMathNode(VfxNodeMathSin, kType_Sin, "math.sin");
DefineMathNode(VfxNodeMathCos, kType_Cos, "math.cos");
DefineMathNode(VfxNodeMathAbs, kType_Abs, "math.abs");
DefineMathNode(VfxNodeMathMin, kType_Min, "math.min");
DefineMathNode(VfxNodeMathMax, kType_Max, "math.max");
DefineMathNode(VfxNodeMathSat, kType_Sat, "math.saturate");
DefineMathNode(VfxNodeMathNeg, kType_Neg, "math.negate");
DefineMathNode(VfxNodeMathSqrt, kType_Sqrt, "math.sqrt");
DefineMathNode(VfxNodeMathPow, kType_Pow, "math.pow");
DefineMathNode(VfxNodeMathExp, kType_Exp, "math.exp");
DefineMathNode(VfxNodeMathMod, kType_Mod, "math.mod");
DefineMathNode(VfxNodeMathFract, kType_Fract, "math.fract");
DefineMathNode(VfxNodeMathFloor, kType_Floor, "math.floor");
DefineMathNode(VfxNodeMathCeil, kType_Ceil, "math.ceil");
DefineMathNode(VfxNodeMathRound, kType_Round, "math.round");
DefineMathNode(VfxNodeMathSign, kType_Sign, "math.sign");
DefineMathNode(VfxNodeMathHypot, kType_Hypot, "math.hypot");
DefineMathNode(VfxNodeMathPitch, kType_Pitch, "math.pitch");
DefineMathNode(VfxNodeMathSemitone, kType_Semitone, "math.semitone");

#undef DefineMathNode
