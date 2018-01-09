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

template <AudioNodeMath::Type _type>
inline float evalMathOp(const float a, const float b, const AudioNodeMath::Type type2)
{
	const AudioNodeMath::Type type = _type == AudioNodeMath::kType_Unknown ? type2 : _type;
	
	switch (type)
	{
	case AudioNodeMath::kType_Unknown:
		return 0.f;
		
	case AudioNodeMath::kType_Add:
		return a + b;
	
	case AudioNodeMath::kType_Sub:
		return a - b;
		
	case AudioNodeMath::kType_Mul:
		return a * b;
		
	case AudioNodeMath::kType_Sin:
		return sinf(a * twoPi);
		
	case AudioNodeMath::kType_Cos:
		return cosf(a * twoPi);
		
	case AudioNodeMath::kType_Abs:
		return fabsf(a);
		
	case AudioNodeMath::kType_Min:
		return fminf(a, b);
		
	case AudioNodeMath::kType_Max:
		return fmaxf(a, b);
		
	case AudioNodeMath::kType_Sat:
		return fmaxf(0.f, fminf(1.f, a));
		
	case AudioNodeMath::kType_Neg:
		return -a;
		
	case AudioNodeMath::kType_Sqrt:
		return sqrtf(a);
		
	case AudioNodeMath::kType_Pow:
		return powf(a, b);
		
	case AudioNodeMath::kType_Exp:
		return expf(a);
		
	case AudioNodeMath::kType_Mod:
		return fmodf(a, b);
		
	case AudioNodeMath::kType_Fract:
		if (a >= 0.f)
			return a - floorf(a);
		else
			return a - ceilf(a);
		
	case AudioNodeMath::kType_Floor:
		return floorf(a);
		
	case AudioNodeMath::kType_Ceil:
		return ceilf(a);
		
	case AudioNodeMath::kType_Round:
		return roundf(a);
		
	case AudioNodeMath::kType_Sign:
		return a < 0.f ? -1.f : +1.f;
		
	case AudioNodeMath::kType_Hypot:
		return hypotf(a, b);
	
	case AudioNodeMath::kType_Pitch:
		return a * powf(2.f, b);
		
	case AudioNodeMath::kType_Semitone:
		return a * powf(2.f, b / 12.f);
	}
	
	return 0.f;
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
		result.set(*a);
	}
	else if (a->isScalar && b->isScalar)
	{
		result.setScalar(evalMathOp<kType_Unknown>(a->getScalar(), b->getScalar(), type));
	}
	else
	{
		a->expand();
		b->expand();
		
		result.setVector();
		
		float * __restrict resultPtr = result.samples;
		
	#define CASE(type) \
		case type: \
			for (int i = 0; i < AUDIO_UPDATE_SIZE; ++i) \
				resultPtr[i] = evalMathOp<type>(a->samples[i], b->samples[i], kType_Unknown); \
			break
		
		switch (type)
		{
		CASE(kType_Unknown);
		CASE(kType_Add);
		CASE(kType_Sub);
		CASE(kType_Mul);
		CASE(kType_Sin);
		CASE(kType_Cos);
		CASE(kType_Abs);
		CASE(kType_Min);
		CASE(kType_Max);
		CASE(kType_Sat);
		CASE(kType_Neg);
		CASE(kType_Sqrt);
		CASE(kType_Pow);
		CASE(kType_Exp);
		CASE(kType_Mod);
		CASE(kType_Fract);
		CASE(kType_Floor);
		CASE(kType_Ceil);
		CASE(kType_Round);
		CASE(kType_Sign);
		CASE(kType_Hypot);
		CASE(kType_Pitch);
		CASE(kType_Semitone);
		}
	}
}

//

AudioNodeMathBase::AudioNodeMathBase()
	: AudioNodeBase()
	, result(0.f)
{
	resizeSockets(kInput_COUNT, kOutput_COUNT);
	addInput(kInput_A, kAudioPlugType_FloatVec);
	addInput(kInput_B, kAudioPlugType_FloatVec);
	addOutput(kOutput_R, kAudioPlugType_FloatVec, &result);
}

//

#define DefineMathNode(name, type, _typeName, eval) \
struct name : AudioNodeMathBase \
{ \
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

DefineMathNode(AudioNodeMathAdd, kType_Add, "math.add", [](float a, float b) -> float { return a + b; });
DefineMathNode(AudioNodeMathSub, kType_Sub, "math.sub", [](float a, float b) -> float { return a - b; });
DefineMathNode(AudioNodeMathMul, kType_Mul, "math.mul", [](float a, float b) -> float { return a * b; });
DefineMathNode(AudioNodeMathSin, kType_Sin, "math.sin", [](float a, float b) -> float { return sinf(a); });
DefineMathNode(AudioNodeMathCos, kType_Cos, "math.cos", [](float a, float b) -> float { return cosf(a); });
DefineMathNode(AudioNodeMathAbs, kType_Abs, "math.abs", [](float a, float b) -> float { return fabsf(a); });
DefineMathNode(AudioNodeMathMin, kType_Min, "math.min", [](float a, float b) -> float { return fminf(a, b); });
DefineMathNode(AudioNodeMathMax, kType_Max, "math.max", [](float a, float b) -> float { return fmaxf(a, b); });
DefineMathNode(AudioNodeMathSat, kType_Sat, "math.saturate", [](float a, float b) -> float { return fmaxf(0.f, fminf(1.f, a)); });
DefineMathNode(AudioNodeMathNeg, kType_Neg, "math.negate", [](float a, float b) -> float { return -a; });
DefineMathNode(AudioNodeMathSqrt, kType_Sqrt, "math.sqrt", [](float a, float b) -> float { return sqrtf(a); });
DefineMathNode(AudioNodeMathPow, kType_Pow, "math.pow", [](float a, float b) -> float { return powf(a, b); });
DefineMathNode(AudioNodeMathExp, kType_Exp, "math.exp", [](float a, float b) -> float { return expf(a); });
DefineMathNode(AudioNodeMathMod, kType_Mod, "math.mod", [](float a, float b) -> float { return fmodf(a, b); });
DefineMathNode(AudioNodeMathFract, kType_Fract, "math.fract", [](float a, float b) -> float { \
	if (a >= 0.f) \
		return a - floorf(a); \
	else \
		return a - ceilf(a); \
});
DefineMathNode(AudioNodeMathFloor, kType_Floor, "math.floor", [](float a, float b) -> float { return floorf(a); });
DefineMathNode(AudioNodeMathCeil, kType_Ceil, "math.ceil", [](float a, float b) -> float { return ceilf(a); });
DefineMathNode(AudioNodeMathRound, kType_Round, "math.round", [](float a, float b) -> float { return roundf(a); });
DefineMathNode(AudioNodeMathSign, kType_Sign, "math.sign", [](float a, float b) -> float { return a < 0.f ? -1.f : +1.f; });
DefineMathNode(AudioNodeMathHypot, kType_Hypot, "math.hypot", [](float a, float b) -> float { return hypotf(a,b); });
DefineMathNode(AudioNodeMathPitch, kType_Pitch, "math.pitch", [](float a, float b) -> float { return a * powf(2.f, b); });
DefineMathNode(AudioNodeMathSemitone, kType_Semitone, "math.semitone", [](float a, float b) -> float { return a * powf(2.f, b / 12.f); });

#undef DefineMathNode
