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

#include "audioNodeBinauralizer.h"

AUDIO_NODE_TYPE(binauralizer, AudioNodeBinauralizer)
{
	typeName = "binauralizer";
	
	in("audio", "audioValue");
	in("sampleSet", "string");
	in("elevation", "audioValue");
	in("azimuth", "audioValue");
	out("leftEar", "audioValue");
	out("rightEar", "audioValue");
}

void AudioNodeBinauralizer::tick(const float dt)
{
	const AudioFloat * audio = getInputAudioFloat(kInput_Audio, &AudioFloat::Zero);
	const char * location = getInputString(kInput_SampleSetLocation, "");
	const float elevation = getInputAudioFloat(kInput_Elevation, &AudioFloat::Zero)->getMean();
	const float azimuth = getInputAudioFloat(kInput_Azimuth, &AudioFloat::Zero)->getMean();

	if (isPassthrough)
	{
		audioOutputL.set(*audio);
		audioOutputR.set(*audio);
		
		return;
	}
	
	if (location != sampleSetLocation)
	{
		sampleSetLocation = std::string("binaural/CIPIC/") + location;
		
		//
		
		sampleSet = binaural::HRIRSampleSet();
		
		binaural::loadHRIRSampleSet_Cipic(sampleSetLocation.c_str(), sampleSet);
		
		sampleSet.finalize();
		
		binauralizer.init(&sampleSet, &mutex);
	}
	
	binauralizer.setSampleLocation(elevation, azimuth);
	
	audio->expand();
	binauralizer.provide(audio->samples, AUDIO_UPDATE_SIZE);
	
	audioOutputL.setVector();
	audioOutputR.setVector();
	
	binauralizer.generateLR(audioOutputL.samples, audioOutputR.samples, AUDIO_UPDATE_SIZE);
}
