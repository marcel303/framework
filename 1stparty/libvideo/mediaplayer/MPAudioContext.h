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
		~AudioContext();

		bool Initialize(
			Context * context,
			const size_t streamIndex,
			const AudioOutputMode outputMode);
		bool Destroy();

		size_t GetStreamIndex() const;

		bool FillAudioBuffer();
		bool RequestAudio(
			int16_t * out_samples,
			const size_t frameCount,
			bool   & out_gotAudio,
			double & out_audioTime);

		bool IsQueueFull() const;
		bool AddPacket(AVPacket & packet);
		bool ProcessPacket(AVPacket & packet);
		bool Depleted() const;
		
		int GetSampleRate() const;
		int GetOutputChannelCount() const;
		
		void ClearBuffers();

	private:
		PacketQueue    * m_packetQueue  = nullptr;
		AudioBuffer    * m_audioBuffer  = nullptr;
		AVCodecContext * m_codecContext = nullptr;
		AVCodec        * m_codec        = nullptr;
		SwrContext     * m_swrContext   = nullptr;
		
		int    m_outputChannelCount = 0;
		double m_timeBase = 0.0;

		size_t m_streamIndex = -1;

		bool m_initialized = false;
	};
};
