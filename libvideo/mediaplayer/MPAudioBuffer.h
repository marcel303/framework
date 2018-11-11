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

#pragma once

#include "ColArray.h"
#include "MPMutex.h"
#include <list>
#include <stdint.h>

#define AVCODEC_MAX_AUDIO_FRAME_SIZE (16 * 1024)

namespace MP
{
	class AudioBufferSegment
	{
	public:
		AudioBufferSegment()
			: m_numSamples(0)
			, m_readOffset(0)
			, m_time(0.0)
		{
		}

		int16_t m_samples[AVCODEC_MAX_AUDIO_FRAME_SIZE/2];
		int m_numSamples;
		int m_readOffset;
		double m_time;
	};

	class AudioBuffer
	{
	public:
		AudioBuffer();

		void AddSegment(const AudioBufferSegment & segment);
		bool ReadSamples(int16_t * __restrict samples, size_t & sampleCount, double & timeStamp);

		bool Depleted() const;
		void Clear();

	private:
		mutable Mutex m_mutex;

		std::list<AudioBufferSegment> m_segments;
	};
};
