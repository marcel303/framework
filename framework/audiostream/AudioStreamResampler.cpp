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

#include "AudioStreamResampler.h"

#define RESAMPLE_FIXEDBITS 32

void AudioStreamResampler::SetSource(AudioStream * source, const int sourceRate, const int targetRate)
{
	mSource = source;

	mBufferPosition_fp = 0;
	mBufferIncrement_fp = (int64_t(sourceRate) << RESAMPLE_FIXEDBITS) / targetRate;
}

int AudioStreamResampler::Provide(int numSamples, AudioSample* __restrict samples)
{
	if (mSource == 0)
		return 0;
	
	int i = 0;
	while (i < numSamples)
	{
		const int bufferPosition = mBufferPosition_fp >> RESAMPLE_FIXEDBITS;

		if (bufferPosition >= mBufferSize)
		{
			mBufferSize = mSource->Provide(kBufferSize, mBuffer);
			if (mBufferSize == 0)
				break;
			mBufferPosition_fp -= int64_t(bufferPosition) << RESAMPLE_FIXEDBITS;
			continue;
		}

		samples[i++] = mBuffer[bufferPosition];
		mBufferPosition_fp += mBufferIncrement_fp;
	}

	return i;
}
