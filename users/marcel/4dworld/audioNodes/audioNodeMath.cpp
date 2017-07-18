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

#include "audioNodeMath.h"
#include <cmath>

static const float twoPi = M_PI * 2.f;

static float evalMathOp(const float a, const float b, const AudioNodeMath::Type type, const bool isPassthrough)
{
	float r = 0.f;
	
	switch (type)
	{
	case AudioNodeMath::kType_Unknown:
		r = 0.f;
		break;
		
	case AudioNodeMath::kType_Add:
		r = a + b;
		break;
	
	case AudioNodeMath::kType_Sub:
		r = a - b;
		break;
		
	case AudioNodeMath::kType_Mul:
		r = a * b;
		break;
		
	case AudioNodeMath::kType_Sin:
		if (isPassthrough)
			r = a;
		else
			r = std::sin(a * twoPi);
		break;
		
	case AudioNodeMath::kType_Cos:
		if (isPassthrough)
			r = a;
		else
			r = std::cos(a * twoPi);
		break;
		
	case AudioNodeMath::kType_Abs:
		if (isPassthrough)
			r = a;
		else
			r = std::abs(a);
		break;
		
	case AudioNodeMath::kType_Min:
		r = std::min(a, b);
		break;
		
	case AudioNodeMath::kType_Max:
		r = std::max(a, b);
		break;
		
	case AudioNodeMath::kType_Sat:
		if (isPassthrough)
			r = a;
		else
			r = std::max(0.f, std::min(1.f, a));
		break;
		
	case AudioNodeMath::kType_Neg:
		if (isPassthrough)
			r = a;
		else
			r = -a;
		break;
		
	case AudioNodeMath::kType_Sqrt:
		if (isPassthrough)
			r = a;
		else
			r = std::sqrt(a);
		break;
		
	case AudioNodeMath::kType_Pow:
		if (isPassthrough)
			r = a;
		else
			r = std::pow(a, b);
		break;
		
	case AudioNodeMath::kType_Exp:
		if (isPassthrough)
			r = a;
		else
			r = std::exp(a);
		break;
		
	case AudioNodeMath::kType_Mod:
		if (isPassthrough)
			r = a;
		else
			r = std::fmod(a, b);
		break;
		
	case AudioNodeMath::kType_Fract:
		if (isPassthrough)
			r = a;
		else if (a >= 0.f)
			r = a - std::floor(a);
		else
			r = a - std::ceil(a);
		break;
		
	case AudioNodeMath::kType_Floor:
		if (isPassthrough)
			r = a;
		else
			r = std::floor(a);
		break;
		
	case AudioNodeMath::kType_Ceil:
		if (isPassthrough)
			r = a;
		else
			r = std::ceil(a);
		break;
		
	case AudioNodeMath::kType_Round:
		if (isPassthrough)
			r = a;
		else
			r = std::round(a);
		break;
		
	case AudioNodeMath::kType_Sign:
		if (isPassthrough)
			r = a;
		else
			r = a < 0.f ? -1.f : +1.f;
		break;
		
	case AudioNodeMath::kType_Hypot:
		r = std::hypot(a, b);
		break;
	
	case AudioNodeMath::kType_Pitch:
		if (isPassthrough)
			r = a;
		else
			r = a * std::powf(2.f, b);
		break;
		
	case AudioNodeMath::kType_Semitone:
		if (isPassthrough)
			r = a;
		else
			r = a * std::powf(2.f, b / 12.f);
		break;
	}
	
	return r;
};

//

AUDIO_ENUM_TYPE(mathType)
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

AUDIO_NODE_TYPE(math, AudioNodeMath)
{
	typeName = "math";
	
	inEnum("type", "mathType", "1");
	in("a", "audioValue");
	in("b", "audioValue");
	out("result", "audioValue");
}

AudioNodeMath::AudioNodeMath()
	: AudioNodeBase()
	, result(0.f)
{
	resizeSockets(kInput_COUNT, kOutput_COUNT);
	addInput(kInput_Type, kAudioPlugType_Int);
	addInput(kInput_A, kAudioPlugType_FloatVec);
	addInput(kInput_B, kAudioPlugType_FloatVec);
	addOutput(kOutput_R, kAudioPlugType_FloatVec, &result);
}

void AudioNodeMath::draw()
{
	const Type type = (Type)getInputInt(kInput_Type, kType_Add);
	const AudioFloat * a = getInputAudioFloat(kInput_A, &AudioFloat::Zero);
	const AudioFloat * b = getInputAudioFloat(kInput_B, &AudioFloat::Zero);
	
	if (a->isScalar && b->isScalar)
	{
		result.setScalar(evalMathOp(a->getScalar(), b->getScalar(), type, isPassthrough));
	}
	else
	{
		a->expand();
		b->expand();
		
		result.setVector();
		
		for (int i = 0; i < AUDIO_UPDATE_SIZE; ++i)
		{
			result.samples[i] = evalMathOp(a->samples[i], b->samples[i], type, isPassthrough);
		}
	}
}

//

AudioNodeMathBase::AudioNodeMathBase(AudioNodeMath::Type _type)
	: AudioNodeBase()
	, type(AudioNodeMath::kType_Unknown)
	, result(0.f)
{
	type = _type;
	
	resizeSockets(kInput_COUNT, kOutput_COUNT);
	addInput(kInput_A, kAudioPlugType_FloatVec);
	addInput(kInput_B, kAudioPlugType_FloatVec);
	addOutput(kOutput_R, kAudioPlugType_FloatVec, &result);
}

void AudioNodeMathBase::draw()
{
	const AudioFloat * a = getInputAudioFloat(kInput_A, &AudioFloat::Zero);
	const AudioFloat * b = getInputAudioFloat(kInput_B, &AudioFloat::Zero);
	
		if (a->isScalar && b->isScalar)
	{
		result.setScalar(evalMathOp(a->getScalar(), b->getScalar(), type, isPassthrough));
	}
	else
	{
		a->expand();
		b->expand();
		
		result.setVector();
		
		for (int i = 0; i < AUDIO_UPDATE_SIZE; ++i)
		{
			result.samples[i] = evalMathOp(a->samples[i], b->samples[i], type, isPassthrough);
		}
	}
}

//

#define DefineMathNode(name, type, _typeName) \
struct name : AudioNodeMathBase \
{ \
	name() \
		: AudioNodeMathBase(AudioNodeMath::type) \
	{ \
	} \
}; \
AUDIO_NODE_TYPE(name, name) \
{ \
	typeName = _typeName; \
	in("a", "audioValue"); \
	in("b", "audioValue"); \
	out("result", "audioValue"); \
}

DefineMathNode(AudioNodeMathAdd, kType_Add, "math.add");
DefineMathNode(AudioNodeMathSub, kType_Sub, "math.sub");
DefineMathNode(AudioNodeMathMul, kType_Mul, "math.mul");
DefineMathNode(AudioNodeMathSin, kType_Sin, "math.sin");
DefineMathNode(AudioNodeMathCos, kType_Cos, "math.cos");
DefineMathNode(AudioNodeMathAbs, kType_Abs, "math.abs");
DefineMathNode(AudioNodeMathMin, kType_Min, "math.min");
DefineMathNode(AudioNodeMathMax, kType_Max, "math.max");
DefineMathNode(AudioNodeMathSat, kType_Sat, "math.saturate");
DefineMathNode(AudioNodeMathNeg, kType_Neg, "math.negate");
DefineMathNode(AudioNodeMathSqrt, kType_Sqrt, "math.sqrt");
DefineMathNode(AudioNodeMathPow, kType_Pow, "math.pow");
DefineMathNode(AudioNodeMathExp, kType_Exp, "math.exp");
DefineMathNode(AudioNodeMathMod, kType_Mod, "math.mod");
DefineMathNode(AudioNodeMathFract, kType_Fract, "math.fract");
DefineMathNode(AudioNodeMathFloor, kType_Floor, "math.floor");
DefineMathNode(AudioNodeMathCeil, kType_Ceil, "math.ceil");
DefineMathNode(AudioNodeMathRound, kType_Round, "math.round");
DefineMathNode(AudioNodeMathSign, kType_Sign, "math.sign");
DefineMathNode(AudioNodeMathHypot, kType_Hypot, "math.hypot");
DefineMathNode(AudioNodeMathPitch, kType_Pitch, "math.pitch");
DefineMathNode(AudioNodeMathSemitone, kType_Semitone, "math.semitone");

#undef DefineMathNode
