#pragma once

#include "MPAudioContext.h"
#include "MPVideoContext.h"
#include "Types.h"
#include <ffmpeg_old/avcodec.h>
#include <stdint.h>

namespace MP
{
	class Context
	{
	public:
		Context();

		bool Begin(const std::string& filename);
		bool End();

		bool HasBegun() const { return m_begun; }

		bool HasAudioStream() const;
		bool HasVideoStream() const; // TODO: Make media player check these flags. Use them to determine master clock.
		bool HasReachedEOF() const;

		size_t GetVideoWidth() const;
		size_t GetVideoHeight() const;

		size_t GetAudioFrameRate() const;
		size_t GetAudioChannelCount() const;
		double GetAudioTime() const;

		bool RequestAudio(int16_t* out_samples, size_t frameCount, bool& out_gotAudio);
		bool RequestVideo(double time, VideoFrame** out_frame, bool& out_gotVideo);

		bool FillBuffers();

		bool SeekToStart();
		bool SeekToTime(double time);

		AVFormatContext* GetFormatContext();

	private:
		bool ReadFormatContext();
		bool GetStreamIndices(size_t& out_audioStreamIndex, size_t& out_videoStreamIndex);
		bool NextPacket();
		bool ProcessPacket(AVPacket& packet);
		bool ReadPacket(AVPacket& out_packet);

		AVFormatContext* m_formatContext;
		AudioContext* m_audioContext;
		VideoContext* m_videoContext;

		bool m_begun;
		std::string m_filename;
		bool m_eof;
		double m_time;
	};
};
