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

#include "MPForward.h"
#include <stdlib.h>

struct SwrContext;

namespace MP
{
	class Context;

	class AudioContext
	{
	public:
		AudioContext();
		~AudioContext();

		bool Initialize(Context * context, const size_t streamIndex);
		bool Destroy();

		size_t GetStreamIndex() const;
		double GetTime() const;

		bool FillAudioBuffer();
		bool RequestAudio(int16_t * out_samples, const size_t frameCount, bool & out_gotAudio);

		bool IsQueueFull() const;
		bool AddPacket(AVPacket & packet);
		bool ProcessPacket(AVPacket & packet);
		bool Depleted() const;

	//private: // FIXME.
		PacketQueue * m_packetQueue;
		AudioBuffer * m_audioBuffer;
		AVCodecContext * m_codecContext;
		AVCodec * m_codec;
		SwrContext * m_swrContext;

		size_t m_streamIndex;
		double m_time;
		size_t m_frameTime;

		bool m_initialized;
	};
};
