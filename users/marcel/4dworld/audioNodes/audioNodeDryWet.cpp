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

#include "audioNodeDryWet.h"
#include "soundmix.h"

AUDIO_NODE_TYPE(drywet, AudioNodeDryWet)
{
	typeName = "audio.drywet";
	
	in("dry", "audioValue");
	in("wet", "audioValue");
	in("wetness", "audioValue");
	out("audio", "audioValue");
}

void AudioNodeDryWet::tick(const float dt)
{
	const AudioFloat * dry = getInputAudioFloat(kInput_Dry, &AudioFloat::Zero);
	const AudioFloat * wet = getInputAudioFloat(kInput_Wet, &AudioFloat::Zero);
	const AudioFloat * wetness = getInputAudioFloat(kInput_Wetness, &AudioFloat::Zero);

	if (isPassthrough)
	{
		audioOutput.set(*dry);
		
		return;
	}

	if (dry->isScalar && wet->isScalar && wetness->isScalar)
	{
		const float dryness = 1.f - wetness->getScalar();

		audioOutput.setScalar(
			dry->getScalar() * dryness +
			wet->getScalar() * wetness->getScalar());
	}
	else if (wetness->isScalar)
	{
		dry->expand();
		wet->expand();

		audioBufferDryWet(audioOutput.samples, dry->samples, wet->samples, AUDIO_UPDATE_SIZE, wetness->getScalar());
	}
	else
	{
		dry->expand();
		wet->expand();
		
		audioBufferDryWet(audioOutput.samples, dry->samples, wet->samples, AUDIO_UPDATE_SIZE, wetness->samples);
	}
}
