#include "MPContext.h"
#include "MPDebug.h"
#include "MPUtil.h"
#include <algorithm>
#include <vector>

#define STREAM_NOT_FOUND 0xFFFF

namespace MP
{
	Context::Context()
		: m_begun(false)
		, m_eof(false)
		, m_time(0.0)
		, m_formatContext(nullptr)
		, m_audioContext(nullptr)
		, m_videoContext(nullptr)
	{
	}

	bool Context::Begin(const std::string & filename, const bool enableAudioStream, const bool enableVideoStream, const bool outputYuv)
	{
		Assert(m_begun == false);

		bool result = true;

		m_begun = true;
		m_filename = filename;
		m_eof = false;
		m_time = 0.0;

		Util::InitializeLibAvcodec();

		// Display libavcodec info.
		Debug::Print("libavcodec: version: %x.", avcodec_version());
		Debug::Print("libavcodec: build: %x.", LIBAVCODEC_BUILD);

		if (result)
		{
			// Open file using AV codec.
			if (av_open_input_file(&m_formatContext, filename.c_str(), 0, 0, 0) != 0)
			{
				Debug::Print("Failed to open input file: %s.", filename.c_str());
				result &= false;
			}
		}

		size_t audioStreamIndex = STREAM_NOT_FOUND;
		size_t videoStreamIndex = STREAM_NOT_FOUND;

		if (result)
		{
			if (GetStreamIndices(audioStreamIndex, videoStreamIndex) != true)
			{
				Debug::Print("Failed to get stream indices.");
				result &= false;
			}
		}

		if (result)
		{
			if (audioStreamIndex == STREAM_NOT_FOUND)
			{
				Debug::Print("No audio stream index found.");
			}
			else if (enableAudioStream)
			{
				// Initialize audio stream/context.
				m_audioContext = new AudioContext;
				if (!m_audioContext->Initialize(this, audioStreamIndex))
					result &= false;
			}
		}

		if (result)
		{
			if (videoStreamIndex == STREAM_NOT_FOUND)
			{
				Debug::Print("No video stream index found.");
			}
			else if (enableVideoStream)
			{
				// Initialize video stream/context.
				m_videoContext = new VideoContext;
				if (!m_videoContext->Initialize(this, videoStreamIndex, outputYuv))
					result &= false;
			}
		}

		return result;
	}

	bool Context::End()
	{
		Assert(m_begun == true);

		bool result = true;

		m_begun = false;

		// Destroy audio context.
		if (m_audioContext)
		{
			Debug::Print("Destroying audio context.");

			m_audioContext->Destroy();
			delete m_audioContext;
			m_audioContext = nullptr;
		}

		// Destroy video context.
		if (m_videoContext)
		{
			Debug::Print("Destroying video context.");

			m_videoContext->Destroy();
			delete m_videoContext;
			m_videoContext = nullptr;
		}

		// Close file using AV codec.
		if (m_formatContext)
		{
			Debug::Print("Closing input file.");

			av_close_input_file(m_formatContext);
			m_formatContext = nullptr;
		}

		return result;
	}

	bool Context::HasAudioStream() const
	{
		if (m_audioContext != nullptr)
			return true;
		else
			return false;
	}

	bool Context::HasVideoStream() const
	{
		if (m_videoContext != nullptr)
			return true;
		else
			return false;
	}

	bool Context::HasReachedEOF() const
	{
		return m_eof;
	}

	size_t Context::GetVideoWidth() const
	{
		Assert(m_begun == true);

		if (m_videoContext)
			return m_videoContext->m_codecContext->width;
		else
			return 0;
	}

	size_t Context::GetVideoHeight() const
	{
		Assert(m_begun == true);

		if (m_videoContext)
			return m_videoContext->m_codecContext->height;
		else
			return 0;
	}

	size_t Context::GetAudioFrameRate() const
	{
		Assert(m_begun == true);

		if (m_audioContext)
			return m_audioContext->m_codecContext->sample_rate;
		else
			return 0;
	}

	size_t Context::GetAudioChannelCount() const
	{
		Assert(m_begun == true);

		if (m_audioContext)
			return m_audioContext->m_codecContext->channels;
		else
			return 0;
	}

	double Context::GetAudioTime() const
	{
		if (m_audioContext)
			return m_audioContext->GetTime();
		else
			return 0.0;
	}

	bool Context::RequestAudio(int16_t * __restrict out_samples, const size_t frameCount, bool & out_gotAudio)
	{
		Assert(m_begun == true);
		Assert(m_audioContext);

		bool result = true;

		if (m_audioContext == nullptr)
			result = false;
		else if (m_audioContext->RequestAudio(out_samples, frameCount, out_gotAudio) != true)
			result = false;

		return result;
	}

	bool Context::RequestVideo(const double time, VideoFrame ** out_frame, bool & out_gotVideo)
	{
		Assert(m_begun == true);
		Assert(m_videoContext);

		bool result = true;

		if (m_videoContext == nullptr)
			result = false;
		else if (m_videoContext->RequestVideo(time, out_frame, out_gotVideo) != true)
			result = false;

		return result;
	}

	bool Context::FillBuffers()
	{
		bool result = true;

		bool full = false;
		
		do
		{
			if (m_audioContext != nullptr)
				if (m_audioContext->IsQueueFull())
					full = true;

			if (m_videoContext != nullptr)
				if (m_videoContext->IsQueueFull())
					full = true;

			if (full == false)
			{
				if (NextPacket() != true)
				{
					result = false;
					Assert(0);
				}
			}
		} while (full == false && m_eof == false);

		return result;
	}

	bool Context::Depleted() const
	{
		if (!HasReachedEOF())
			return false;
		if (m_audioContext && !m_audioContext->Depleted())
			return false;
		if (m_videoContext && !m_videoContext->Depleted())
			return false;
		return true;
	}

	void Context::FillVideoBuffer()
	{
		if (m_videoContext)
			m_videoContext->FillVideoBuffer();
	}

	bool Context::SeekToStart()
	{
		bool result = true;

		std::vector<size_t> streams;

		if (m_audioContext != nullptr)
		{
			streams.push_back(m_audioContext->GetStreamIndex());
			m_audioContext->m_packetQueue.Clear();
			m_audioContext->m_audioBuffer.Clear();
		}

		if (m_videoContext != nullptr)
		{
			streams.push_back(m_videoContext->GetStreamIndex());
			m_videoContext->m_packetQueue.Clear();
			m_videoContext->m_videoBuffer.Clear();
			m_videoContext->m_time = 0.0;
		}

		for (size_t i = 0; i < streams.size(); ++i)
		{
			if (av_seek_frame(m_formatContext, streams[i], 0, 0*AVSEEK_FLAG_BYTE | 0*AVSEEK_FLAG_ANY) < 0)
				result = false;
		}

		return result;
	}

	bool Context::SeekToTime(const double time)
	{
		bool result = true;

		int streamIndex = -1;

		if (false)
		{
			if (m_videoContext)
				streamIndex = m_videoContext->GetStreamIndex();
			else if (m_audioContext)
				streamIndex = m_audioContext->GetStreamIndex();
		}

		if (av_seek_frame(m_formatContext, streamIndex, time * AV_TIME_BASE, (1*AVSEEK_FLAG_ANY)) < 0)
			result = false;

		if (m_audioContext != nullptr)
		{
			m_audioContext->m_packetQueue.Clear();
			m_audioContext->m_audioBuffer.Clear();
			avcodec_flush_buffers(m_audioContext->m_codecContext);
		}

		if (m_videoContext != nullptr)
		{
			m_videoContext->m_packetQueue.Clear();
			m_videoContext->m_videoBuffer.Clear();
			m_videoContext->m_time = time;
			avcodec_flush_buffers(m_videoContext->m_codecContext);
		}
		
		return result;
	}

	AVFormatContext * Context::GetFormatContext()
	{
		return m_formatContext;
	}

	bool Context::GetStreamIndices(size_t & out_audioStreamIndex, size_t & out_videoStreamIndex)
	{
		out_audioStreamIndex = STREAM_NOT_FOUND;
		out_videoStreamIndex = STREAM_NOT_FOUND;

		// Get stream info.
		if (av_find_stream_info(m_formatContext) < 0)
		{
			Debug::Print("unable to get stream info");
			return false;
		}

		// Show stream info.
		dump_format(m_formatContext, 0, m_filename.c_str(), false);

		// Find the first audio & video streams and use those during rendering.
		for (size_t i = 0; i < static_cast<size_t>(m_formatContext->nb_streams); ++i)
		{
			if (out_audioStreamIndex == STREAM_NOT_FOUND)
				if (m_formatContext->streams[i]->codec->codec_type == CODEC_TYPE_AUDIO)
					out_audioStreamIndex = i;

			if (out_videoStreamIndex == STREAM_NOT_FOUND)
				if (m_formatContext->streams[i]->codec->codec_type == CODEC_TYPE_VIDEO)
					out_videoStreamIndex = i;
		}

		if (out_audioStreamIndex == STREAM_NOT_FOUND)
			Debug::Print("* No audio stream found.");

		if (out_videoStreamIndex == STREAM_NOT_FOUND)
			Debug::Print("* No video stream found.");

		return true;
	}

	bool Context::NextPacket()
	{
		bool result = true;

		// Read packet.
		AVPacket packet;

		if (ReadPacket(packet))
		{
			// Process packet.
			if (ProcessPacket(packet) != true)
				result = false;
		}
		else
		{
			// Couldn't read another packet- EOF reached.
			if (m_eof == false)
			{
				Debug::Print("Reached EOF.");

				m_eof = true;
			}
			else
			{
				Debug::Print("Attempt to read beyond EOF.");
			}
		}

		return result;
	}

	bool Context::ProcessPacket(AVPacket & packet)
	{
		bool result = true;

		bool captured = false;

		Debug::Print("ProcessPacket: stream_index=%d.", packet.stream_index);

		if (m_audioContext != nullptr)
		{
			if (packet.stream_index == m_audioContext->GetStreamIndex())
			{
				result &= m_audioContext->AddPacket(packet);
				captured = true;
			}
		}

		if (m_videoContext != nullptr)
		{
			if (packet.stream_index == m_videoContext->GetStreamIndex())
			{
				result &= m_videoContext->AddPacket(packet);
				captured = true;
			}
		}

		if (!captured)
		{
			av_free_packet(&packet);
		}

		return result;
	}

	bool Context::ReadPacket(AVPacket & out_packet)
	{
		// Read packet.
		out_packet.data = 0;

		const int readResult = av_read_frame(m_formatContext, &out_packet);

		if (readResult < 0)
			return false;

		if (out_packet.data == 0)
		{
			Assert(0);
			return false;
		}

		return true;
	}
};