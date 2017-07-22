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

#include "audioNodeInterpolateScalar.h"
#include "Noise.h"
#include <cmath>

AUDIO_NODE_TYPE(interpolate_scalar, AudioNodeInterpolateScalar)
{
	typeName = "interp.scalar";
	
	in("value", "audioValue");
	out("result", "audioValue");
}

void AudioNodeInterpolateScalar::tick(const float dt)
{
	const AudioFloat * value = getInputAudioFloat(kInput_Value, &AudioFloat::Zero);
	
	//

	const float oldValue = previousValue;
	const float newValue = value->getScalar();

	const float tStep = 1.f / AUDIO_UPDATE_SIZE;
	float t = 0.f;

	for (int i = 0; i < AUDIO_UPDATE_SIZE; ++i)
	{
		const float w2 = t;
		const float w1 = 1.f - t;

		resultOutput.samples[i] = oldValue * w1 + newValue * w2;

		t += tStep;
	}

	previousValue = newValue;
}
