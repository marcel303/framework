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

		bool Begin(const std::string & filename, const bool enableAudioStream = true, const bool enableVideoStream = true, const bool outputYuv = false);
		bool End();

		bool HasBegun() const { return m_begun; }

		bool HasAudioStream() const;
		bool HasVideoStream() const; // TODO: Make media player check these flags. Use them to determine master clock.
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
