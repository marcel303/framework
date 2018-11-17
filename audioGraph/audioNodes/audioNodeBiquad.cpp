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

#include "audioNodeBiquad.h"

AUDIO_ENUM_TYPE(biquadType)
{
	elem("lowpass");
	elem("highpass");
	elem("bandpass");
	elem("notch");
	elem("peak");
	elem("lowshelf");
	elem("highshelf");
}

AUDIO_NODE_TYPE(AudioNodeBiquad)
{
	typeName = "filter.biquad";
	
	in("in", "audioValue");
	inEnum("type", "biquadType");
	in("frequency", "audioValue", "100");
	in("Q", "audioValue", "0.707");
	in("peakGain", "audioValue", "6");
	out("out", "audioValue");
}

AudioNodeBiquad::AudioNodeBiquad()
	: AudioNodeBase()
	, biquad()
	, resultOutput()
{
	resizeSockets(kInput_COUNT, kOutput_COUNT);
	addInput(kInput_Input, kAudioPlugType_FloatVec);
	addInput(kInput_Type, kAudioPlugType_Int);
	addInput(kInput_Frequency, kAudioPlugType_FloatVec);
	addInput(kInput_Q, kAudioPlugType_FloatVec);
	addInput(kInput_PeakGain, kAudioPlugType_FloatVec);
	addOutput(kOutput_Output, kAudioPlugType_FloatVec, &resultOutput);
}

void AudioNodeBiquad::tick(const float dt)
{
	const AudioFloat defaultFrequency(100.f);
	const AudioFloat defaultQ(.707f);
	const AudioFloat defaultPeakGain(6.f);
	
	const AudioFloat * input = getInputAudioFloat(kInput_Input, &AudioFloat::Zero);
	const Type type = (Type)getInputInt(kInput_Type, 0);
	const AudioFloat * frequency = getInputAudioFloat(kInput_Frequency, &defaultFrequency);
	const AudioFloat * Q_in = getInputAudioFloat(kInput_Q, &defaultQ);
	const AudioFloat * peakGain = getInputAudioFloat(kInput_PeakGain, &defaultPeakGain);
	
	const double Fc = frequency->getMean() / SAMPLE_RATE;
	
	const float Q = Q_in->getMean();
	
	if (isPassthrough)
	{
		resultOutput.set(*input);
	}
	else if (Fc <= 0.0 || Q <= 0.0)
	{
		resultOutput.setZero();
		
		biquad = BiquadFilter<double>();
	}
	else
	{
		switch (type)
		{
		case kType_Lowpass:
			biquad.makeLowpass(Fc, Q, peakGain->getMean());
			break;
			
		case kType_Highpass:
			biquad.makeHighpass(Fc, Q, peakGain->getMean());
			break;
			
		case kType_Bandpass:
			biquad.makeBandpass(Fc, Q, peakGain->getMean());
			break;
			
		case kType_Notch:
			biquad.makeNotch(Fc, Q, peakGain->getMean());
			break;
			
		case kType_Peak:
			biquad.makePeak(Fc, Q, peakGain->getMean());
			break;
			
		case kType_Lowshelf:
			biquad.makeLowshelf(Fc, Q, peakGain->getMean());
			break;
			
		case kType_Highshelf:
			biquad.makeHighshelf(Fc, Q, peakGain->getMean());
			break;
		}
		
		//
		
		input->expand();
		resultOutput.setVector();
		
		for (int i = 0; i < AUDIO_UPDATE_SIZE; ++i)
		{
			const float value = biquad.processSingle(input->samples[i]);
			
			resultOutput.samples[i] = value;
		}
	}
}

bool AudioNodeBiquad::getFilterResponse(float * magnitude, const int numSteps) const
{
	const double a[3] =
	{
		biquad.a0,
		biquad.a1,
		biquad.a2
	};
	
	const double b[3] =
	{
		1.0,
		biquad.b1,
		biquad.b2
	};
	
	evaluateFilter(a, b, 3, 0.0, M_PI, magnitude, numSteps);
	
	return true;
}
