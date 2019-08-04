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

#include "audioNodeImpulseResponse.h"
#include <math.h>

AUDIO_NODE_TYPE(AudioNodeImpulseResponse)
{
	typeName = "impulse.response";
	
	in("value", "audioValue");
	in("frequency", "audioValue");
	out("response", "audioValue");
}

AudioNodeImpulseResponse::AudioNodeImpulseResponse()
	: AudioNodeBase()
	, measurementPhase(0.0)
	, runningImpulseResponseX(0.0)
	, runningImpulseResponseY(0.0)
{
	resizeSockets(kInput_COUNT, kOutput_COUNT);
	addInput(kInput_Value, kAudioPlugType_FloatVec);
	addInput(kInput_Frequency, kAudioPlugType_FloatVec);
	addOutput(kOutput_ImpulseResponse, kAudioPlugType_FloatVec, &impulseResponseOutput);
}

void AudioNodeImpulseResponse::tick(const float _dt)
{
	audioCpuTimingBlock(AudioNodeImpulseResponse);
	
	const AudioFloat * value = getInputAudioFloat(kInput_Value, &AudioFloat::Zero);
	const AudioFloat * frequency = getInputAudioFloat(kInput_Frequency, &AudioFloat::Zero);
	
	value->expand();
	frequency->expand();
	
	//
	
	{
		// todo : decay rate should be an input
		
		const double dt = 1.0 / SAMPLE_RATE;
		const double twoPi = 2.0 * M_PI;
		const double retain = pow(0.5, dt * 1000.0);
		
		impulseResponseOutput.setVector();
		
		for (int i = 0; i < AUDIO_UPDATE_SIZE; ++i)
		{
			runningImpulseResponseX *= retain;
			runningImpulseResponseY *= retain;
			
			const double measurementValue = value->samples[i];
			const double measurementFrequency = frequency->samples[i];
			
			const double x = cos(measurementPhase * twoPi);
			const double y = sin(measurementPhase * twoPi);
			
			runningImpulseResponseX += measurementValue * x;
			runningImpulseResponseY += measurementValue * y;
			
			measurementPhase = fmod(measurementPhase + measurementFrequency * dt, 1.0);
			
			impulseResponseOutput.samples[i] = float(hypotf(runningImpulseResponseX, runningImpulseResponseY));
		}
	}
}
