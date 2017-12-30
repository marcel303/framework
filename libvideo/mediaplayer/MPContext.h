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
#include <stdint.h>
#include <string>

namespace MP
{
	class Context
	{
	public:
		Context();
		~Context();

		bool Begin(const std::string & filename, const bool enableAudioStream = true, const bool enableVideoStream = true, const OutputMode outputMode = kOutputMode_RGBA);
		bool End();

		bool HasBegun() const { return m_begun; }

		bool HasAudioStream() const;
		bool HasVideoStream() const;
		bool HasReachedEOF() const;
		double GetDuration() const;

		size_t GetVideoWidth() const;
		size_t GetVideoHeight() const;

		size_t GetAudioFrameRate() const;
		size_t GetAudioChannelCount() const;
		double GetAudioTime() const;

		bool RequestAudio(int16_t * __restrict out_samples, const size_t frameCount, bool & out_gotAudio);
		bool RequestVideo(const double time, VideoFrame ** out_frame, bool & out_gotVideo);

		bool FillBuffers();
		bool Depleted() const;

		void FillAudioBuffer();
		void FillVideoBuffer();

		bool SeekToStart();
		bool SeekToTime(const double time);

		AVFormatContext * GetFormatContext();

	private:
		bool GetStreamIndices(size_t & out_audioStreamIndex, size_t & out_videoStreamIndex);
		bool NextPacket();
		bool ProcessPacket(AVPacket & packet);
		bool ReadPacket(AVPacket & out_packet);

		bool m_begun;
		std::string m_filename;
		bool m_eof;
		double m_time;

		AVFormatContext * m_formatContext;
		AudioContext * m_audioContext;
		VideoContext * m_videoContext;
	};
};
