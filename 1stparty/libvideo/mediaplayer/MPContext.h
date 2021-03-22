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
#include <stdint.h>
#include <string>

namespace MP
{
	class Context
	{
	public:
		~Context();

		bool Begin(
			const std::string & filename,
			const bool enableAudioStream = true,
			const bool enableVideoStream = true,
			const OutputMode outputMode = kOutputMode_RGBA,
			const int desiredAudioStreamIndex = -1,
			const AudioOutputMode audioOutputMode = kAudioOutputMode_Stereo);
		bool End();

		bool HasBegun() const { return m_begun; }

		bool HasAudioStream() const;
		bool HasVideoStream() const;
		bool HasReachedEOF() const;
		double GetDuration() const;

		size_t GetVideoWidth() const;
		size_t GetVideoHeight() const;
		double GetVideoSampleAspectRatio() const;

		size_t GetAudioFrameRate() const;
		size_t GetAudioChannelCount() const;

		bool RequestAudio(int16_t * __restrict out_samples, const size_t frameCount, bool & out_gotAudio, double & out_audioTime);
		bool RequestVideo(const double time, VideoFrame ** out_frame, bool & out_gotVideo);

		bool FillBuffers();
		bool Depleted() const;

		void FillAudioBuffer();
		void FillVideoBuffer();

		bool SeekToStart();
		bool SeekToTime(const double time, const bool nearest, double & actualTime);

		AVFormatContext * GetFormatContext();

	private:
		bool GetStreamIndices(
			size_t & out_audioStreamIndex,
			size_t & out_videoStreamIndex,
			const int desiredAudioStreamIndex);
			
		bool NextPacket();
		bool ProcessPacket(AVPacket & packet);
		bool ReadPacket(AVPacket & out_packet);

		bool        m_begun    = false;
		std::string m_filename;
		bool        m_eof      = false;
		double      m_time     = 0.0;

		AVFormatContext * m_formatContext = nullptr;
		AudioContext    * m_audioContext = nullptr;
		VideoContext    * m_videoContext = nullptr;
	};
};
