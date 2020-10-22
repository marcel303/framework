/*
	Copyright (C) 2020 Marcel Smit
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

#include "SoundIO.h"

#include "audiostream/AudioStream.h"
#include "audiostream/AudioStreamVorbis.h"

#include <string.h>
#include <vector>

SoundData * loadSound_OGG(const char * filename)
{
	static const int kMaxSamples = (1 << 14) * sizeof(short);
	AudioSample samples[kMaxSamples];
	
	std::vector<AudioSample> readBuffer;
	
	AudioStream_Vorbis stream;
	stream.Open(filename, false);
	
	if (!stream.IsOpen_get())
		return nullptr;
	
	const int sampleRate = stream.SampleRate_get();
	
	// read kMaxSamples samples at a time until we finished reading the entire stream
	
	for (;;)
	{
		const int numSamples = stream.Provide(kMaxSamples, samples);
		
		if (numSamples == 0)
			break;
		else
		{
			readBuffer.resize(readBuffer.size() + numSamples);
			memcpy(&readBuffer[0] + readBuffer.size() - numSamples, samples, numSamples * sizeof(AudioSample));
		}
	}
	
	stream.Close();
	
	const int numSamples = readBuffer.size();
	const int numBytes = numSamples * sizeof(AudioSample);
	void * bytes = nullptr;
	
	if (numBytes > 0)
	{
		bytes = new char[numBytes];
		memcpy(bytes, &readBuffer[0], numBytes);
	}
	
	SoundData * soundData = new SoundData();
	soundData->channelSize = 2;
	soundData->channelCount = 2;
	soundData->sampleCount = numSamples;
	soundData->sampleRate = sampleRate;
	soundData->sampleData = bytes;
	
	return soundData;
}
