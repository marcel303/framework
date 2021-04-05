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

struct SwsContext;

namespace MP
{
	class Context;
	
	class VideoContext
	{
	public:
		~VideoContext();

		bool Initialize(
			Context * context,
			const size_t streamIndex,
			const OutputMode outputMode);
		bool Destroy();

		size_t GetStreamIndex() const;
		double GetTime() const;

		void FillVideoBuffer();

		bool RequestVideo(const double time, VideoFrame ** out_frame, bool & out_gotVideo);

		bool IsQueueFull() const;
		bool AddPacket(const AVPacket & packet);
		bool ProcessPacket(AVPacket & packet);
		bool Depleted() const;
		
		int GetVideoWidth() const;
		int GetVideoHeight() const;
		
		VideoBuffer * GetVideoBuffer() { return m_videoBuffer; }
		
		void ClearBuffers();

	private:
		bool Convert(VideoFrame * out_frame);
		void SetTimingForFrame(VideoFrame * out_frame);

		PacketQueue    * m_packetQueue     = nullptr;
		AVCodecContext * m_codecContext    = nullptr;
		AVCodec        * m_codec           = nullptr;
		VideoFrame     * m_tempVideoFrame  = nullptr;
		AVFrame        * m_tempFrame       = nullptr;
		uint8_t        * m_tempFrameBuffer = nullptr;
		VideoBuffer    * m_videoBuffer     = nullptr;
		SwsContext     * m_swsContext      = nullptr;
		double           m_timeBase        = 0.0;

		size_t m_streamIndex    = -1;
		OutputMode m_outputMode = kOutputMode_RGBA;
		
		double m_time       = 0.0;
		size_t m_frameCount = 0;

		bool m_initialized = false;
	};
};
