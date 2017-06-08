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
#include "types.h"

struct SwsContext;

namespace MP
{
	class Context;
	
	class VideoContext
	{
	public:
		VideoContext();
		~VideoContext();

		bool Initialize(Context * context, const size_t streamIndex, const OutputMode outputMode);
		bool Destroy();

		size_t GetStreamIndex() const;
		double GetTime() const;

		void FillVideoBuffer();

		bool RequestVideo(const double time, VideoFrame ** out_frame, bool & out_gotVideo);

		bool IsQueueFull() const;
		bool AddPacket(const AVPacket & packet);
		bool ProcessPacket(AVPacket & packet);
		bool Depleted() const;

	//private:
		bool Convert(VideoFrame * out_frame);
		void SetTimingForFrame(VideoFrame * out_frame);

		PacketQueue * m_packetQueue;
		AVCodecContext * m_codecContext;
		AVCodec * m_codec;
		VideoFrame * m_tempVideoFrame;
		AVFrame * m_tempFrame;
		uint8_t * m_tempFrameBuffer;
		VideoBuffer * m_videoBuffer;
		SwsContext * m_swsContext;
		double m_timeBase;

		size_t m_streamIndex;
		OutputMode m_outputMode;
		double m_time;
		size_t m_frameCount;

		bool m_initialized;
	};
};
