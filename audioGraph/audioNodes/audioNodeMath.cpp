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
#include <math.h>

static const float twoPi = M_PI * 2.f;

static float evalMathOp(const float a, const float b, const AudioNodeMath::Type type)
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
		r = sinf(a * twoPi);
		break;
		
	case AudioNodeMath::kType_Cos:
		r = cosf(a * twoPi);
		break;
		
	case AudioNodeMath::kType_Abs:
		r = fabsf(a);
		break;
		
	case AudioNodeMath::kType_Min:
		r = fminf(a, b);
		break;
		
	case AudioNodeMath::kType_Max:
		r = fmaxf(a, b);
		break;
		
	case AudioNodeMath::kType_Sat:
		r = fmaxf(0.f, fminf(1.f, a));
		break;
		
	case AudioNodeMath::kType_Neg:
		r = -a;
		break;
		
	case AudioNodeMath::kType_Sqrt:
		r = sqrtf(a);
		break;
		
	case AudioNodeMath::kType_Pow:
		r = powf(a, b);
		break;
		
	case AudioNodeMath::kType_Exp:
		r = expf(a);
		break;
		
	case AudioNodeMath::kType_Mod:
		r = fmodf(a, b);
		break;
		
	case AudioNodeMath::kType_Fract:
		if (a >= 0.f)
			r = a - floorf(a);
		else
			r = a - ceilf(a);
		break;
		
	case AudioNodeMath::kType_Floor:
		r = floorf(a);
		break;
		
	case AudioNodeMath::kType_Ceil:
		r = ceilf(a);
		break;
		
	case AudioNodeMath::kType_Round:
		r = roundf(a);
		break;
		
	case AudioNodeMath::kType_Sign:
		r = a < 0.f ? -1.f : +1.f;
		break;
		
	case AudioNodeMath::kType_Hypot:
		r = hypotf(a, b);
		break;
	
	case AudioNodeMath::kType_Pitch:
		r = a * powf(2.f, b);
		break;
		
	case AudioNodeMath::kType_Semitone:
		r = a * powf(2.f, b / 12.f);
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

void AudioNodeMath::tick(const float dt)
{
	const Type type = (Type)getInputInt(kInput_Type, kType_Add);
	const AudioFloat * a = getInputAudioFloat(kInput_A, &AudioFloat::Zero);
	const AudioFloat * b = getInputAudioFloat(kInput_B, &AudioFloat::Zero);
	
	if (isPassthrough)
	{
		result.setScalar(0.f);
	}
	else if (a->isScalar && b->isScalar)
	{
		result.setScalar(evalMathOp(a->getScalar(), b->getScalar(), type));
	}
	else
	{
		a->expand();
		b->expand();
		
		result.setVector();
		
		for (int i = 0; i < AUDIO_UPDATE_SIZE; ++i)
		{
			result.samples[i] = evalMathOp(a->samples[i], b->samples[i], type);
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

void AudioNodeMathBase::tick(const float dt)
{
	const AudioFloat * a = getInputAudioFloat(kInput_A, &AudioFloat::Zero);
	const AudioFloat * b = getInputAudioFloat(kInput_B, &AudioFloat::Zero);
	
	if (isPassthrough)
	{
		result.set(*a);
	}
	else if (a->isScalar && b->isScalar)
	{
		result.setScalar(evalMathOp(a->getScalar(), b->getScalar(), type));
	}
	else
	{
		a->expand();
		b->expand();
		
		result.setVector();
		
		for (int i = 0; i < AUDIO_UPDATE_SIZE; ++i)
		{
			result.samples[i] = evalMathOp(a->samples[i], b->samples[i], type);
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

#define DefineMathNode_V2(name, type, _typeName, eval) \
struct name : AudioNodeMathBase \
{ \
	name() \
		: AudioNodeMathBase(AudioNodeMath::type) \
	{ \
	} \
	virtual void tick(const float dt) override \
	{ \
		const AudioFloat * a = getInputAudioFloat(kInput_A, &AudioFloat::Zero); \
		const AudioFloat * b = getInputAudioFloat(kInput_B, &AudioFloat::Zero); \
		if (isPassthrough) \
		{ \
			result.set(*a); \
		} \
		else if (a->isScalar && b->isScalar) \
		{ \
			result.setScalar(eval(a->getScalar(), b->getScalar())); \
		} \
		else \
		{ \
			a->expand(); \
			b->expand(); \
			result.setVector(); \
			float * __restrict dst = result.samples; \
			for (int i = 0; i < AUDIO_UPDATE_SIZE; ++i) \
			{ \
				dst[i] = eval(a->samples[i], b->samples[i]); \
			} \
		} \
	} \
}; \
AUDIO_NODE_TYPE(name, name) \
{ \
	typeName = _typeName; \
	in("a", "audioValue"); \
	in("b", "audioValue"); \
	out("result", "audioValue"); \
}

DefineMathNode_V2(AudioNodeMathAdd, kType_Add, "math.add", [](float a, float b) -> float { return a + b; });
DefineMathNode_V2(AudioNodeMathSub, kType_Sub, "math.sub", [](float a, float b) -> float { return a - b; });
DefineMathNode_V2(AudioNodeMathMul, kType_Mul, "math.mul", [](float a, float b) -> float { return a * b; });
DefineMathNode_V2(AudioNodeMathSin, kType_Sin, "math.sin", [](float a, float b) -> float { return sinf(a); });
DefineMathNode_V2(AudioNodeMathCos, kType_Cos, "math.cos", [](float a, float b) -> float { return cosf(a); });
DefineMathNode_V2(AudioNodeMathAbs, kType_Abs, "math.abs", [](float a, float b) -> float { return fabsf(a); });
DefineMathNode_V2(AudioNodeMathMin, kType_Min, "math.min", [](float a, float b) -> float { return fminf(a, b); });
DefineMathNode_V2(AudioNodeMathMax, kType_Max, "math.max", [](float a, float b) -> float { return fmaxf(a, b); });
DefineMathNode_V2(AudioNodeMathSat, kType_Sat, "math.saturate", [](float a, float b) -> float { return fmaxf(0.f, fminf(1.f, a)); });
DefineMathNode_V2(AudioNodeMathNeg, kType_Neg, "math.negate", [](float a, float b) -> float { return -a; });
DefineMathNode_V2(AudioNodeMathSqrt, kType_Sqrt, "math.sqrt", [](float a, float b) -> float { return sqrtf(a); });
DefineMathNode_V2(AudioNodeMathPow, kType_Pow, "math.pow", [](float a, float b) -> float { return powf(a, b); });
DefineMathNode_V2(AudioNodeMathExp, kType_Exp, "math.exp", [](float a, float b) -> float { return expf(a); });
DefineMathNode_V2(AudioNodeMathMod, kType_Mod, "math.mod", [](float a, float b) -> float { return fmodf(a, b); });
DefineMathNode_V2(AudioNodeMathFract, kType_Fract, "math.fract", [](float a, float b) -> float { \
	if (a >= 0.f) \
		return a - floorf(a); \
	else \
		return a - ceilf(a); \
});
DefineMathNode_V2(AudioNodeMathFloor, kType_Floor, "math.floor", [](float a, float b) -> float { return floorf(a); });
DefineMathNode_V2(AudioNodeMathCeil, kType_Ceil, "math.ceil", [](float a, float b) -> float { return ceilf(a); });
DefineMathNode_V2(AudioNodeMathRound, kType_Round, "math.round", [](float a, float b) -> float { return roundf(a); });
DefineMathNode_V2(AudioNodeMathSign, kType_Sign, "math.sign", [](float a, float b) -> float { return a < 0.f ? -1.f : +1.f; });
DefineMathNode_V2(AudioNodeMathHypot, kType_Hypot, "math.hypot", [](float a, float b) -> float { return hypotf(a,b); });
DefineMathNode_V2(AudioNodeMathPitch, kType_Pitch, "math.pitch", [](float a, float b) -> float { return a * powf(2.f, b); });
DefineMathNode_V2(AudioNodeMathSemitone, kType_Semitone, "math.semitone", [](float a, float b) -> float { return a * powf(2.f, b / 12.f); });

#undef DefineMathNode
#undef DefineMathNode_V2
