#include "Debugging.h"
#include "Ease.h"
#include <math.h>

float EvalEase(float t, EaseType type, float param1)
{
	Assert(t >= 0.f && t <= 1.f);
	
	float result = t;
	
	switch (type)
	{
	case kEaseType_Linear:
		break;
		
	case kEaseType_PowIn:
		result = powf(t, param1);
		break;
		
	case kEaseType_PowOut:
		result = 1.f - powf(1.f - t, param1);
		break;
		
	case kEaseType_PowInOut:
		if ((t *= 2.f) < 1.f)
			result = .5f * powf(t, param1);
		else
			result = 1.f - .5f * fabsf(powf(2.f - t, param1));
		break;
		
	case kEaseType_SineIn:
		result = 1.f - cosf(t * M_PI/2.f);
		break;
		
	case kEaseType_SineOut:
		result = sinf(t * M_PI/2.f);
		break;
		
	case kEaseType_SineInOut:
		result = -.5f * (cosf(M_PI * t) - 1.f);
		break;
		
	case kEaseType_BackIn:
		result = t * t * ((param1 + 1.f) * t - param1);
		break;
		
	case kEaseType_BackOut:
		t -= 1.f;
		//result = (--t * t * ((param1 + 1.f) * t + param1) + 1.f);
		result = (t * t * ((param1 + 1.f) * t + param1) + 1.f);
		break;
		
	case kEaseType_BackInOut:
		if ((t *= 2.f) < 1.f)
			result = .5f * (t * t * ((param1 + 1.f) * t - param1));
		else
		{
			//result = .5 * ((t -= 2.f) * t * ((param1 + 1.f) * t + param1) + 2.f);
			t -= 2.f;
			result = .5 * ((t) * t * ((param1 + 1.f) * t + param1) + 2.f);
		}
		break;
		
	case kEaseType_BounceIn:
		result = 1.f - EvalEase(1.f - t, kEaseType_BounceOut, 0.f);
		break;
		
	case kEaseType_BounceOut:
		if (t < 1.f / 2.75f)
			result = (7.5625f * t * t);
		else if (t < 2.f / 2.75f)
		{
			t -= 1.5f / 2.75f;
			result = (7.5625f * t * t + .75f);
			//result = (7.5625f * (t -= 1.5f / 2.75f) * t + .75f);
		}
		else if (t < 2.5f / 2.75f)
		{
			t -= 2.25f / 2.75f;
			result = (7.5625f * t * t + .9375f);
			//result = (7.5625f * (t -= 2.25f / 2.75f) * t + .9375f);
		}
		else
		{
			t -= 2.625f / 2.75f;
			result = (7.5625f * t * t + .984375f);
			//result = (7.5625f * (t -= 2.625f / 2.75f) * t + .984375f);
		}
		break;
		
	case kEaseType_BounceInOut:
		if (t < .5f)
			result = EvalEase(t * 2.f, kEaseType_BounceIn, 0.f) * .5f;
		else
			result = EvalEase(t * 2.f - 1.f, kEaseType_BounceOut, 0.f) * .5f + .5f;
		break;

		// todo : elastic in/out
		
	default:
		Assert(false);
		break;
	}

	return result;
}
