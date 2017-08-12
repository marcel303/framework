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

#include "audioNodeInput.h"
#include "audioUpdateHandler.h"
#include "Noise.h"
#include <cmath>

AUDIO_NODE_TYPE(input, AudioNodeInput)
{
	typeName = "audio.in";
	
	in("channel", "int");
	in("gain", "audioValue", "1");
	out("audio", "audioValue");
}

void AudioNodeInput::tick(const float dt)
{
	const int channel = getInputInt(kInput_Channel, 0);
	const AudioFloat * gain = getInputAudioFloat(kInput_Gain, &AudioFloat::One);
	
	if (isPassthrough || channel < 0 || channel >= g_numAudioInputChannels)
	{
		audioOutput.setZero();
	}
	else
	{
		const float * __restrict channelPtr = g_audioInputChannels + channel;

		audioOutput.setVector();

		for (int i = 0; i < AUDIO_UPDATE_SIZE; ++i, channelPtr += g_numAudioInputChannels)
		{
			audioOutput.samples[i] = *channelPtr;
		}
		
		audioOutput.mul(*gain);
	}
}
