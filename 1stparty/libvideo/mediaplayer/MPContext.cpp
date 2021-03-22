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

#include "MPAudioBuffer.h"
#include "MPAudioContext.h"
#include "MPContext.h"
#include "MPDebug.h"
#include "MPPacketQueue.h"
#include "MPUtil.h"
#include "MPVideoBuffer.h"
#include "MPVideoContext.h"
#include <algorithm>
#include <vector>

extern "C"
{
	#include <libavcodec/avcodec.h>
	#include <libavformat/avformat.h>
}

#define STREAM_NOT_FOUND 0xFFFF

#if !defined(LIBAVCODEC_VERSION_MAJOR)
	#error LIBAVCODEC_VERSION_MAJOR not defined
#endif

namespace MP
{
	Context::~Context()
	{
		Assert(m_begun == false);
		Assert(m_filename.empty());
		Assert(m_eof   == false);
		Assert(m_time  == 0.0);
		
		Assert(m_formatContext == nullptr);
		Assert(m_audioContext  == nullptr);
		Assert(m_videoContext  == nullptr);
	}

	bool Context::Begin(
		const std::string & filename,
		const bool enableAudioStream,
		const bool enableVideoStream,
		const OutputMode outputMode,
		const int desiredAudioStreamIndex,
		const AudioOutputMode audioOutputMode)
	{
		Assert(m_begun == false);
		Assert(m_filename.empty());
		Assert(m_eof   == false);
		Assert(m_time  == 0.0);

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
			if (avformat_open_input(&m_formatContext, filename.c_str(), nullptr, nullptr) < 0)
			{
				Debug::Print("Failed to open input file: %s.", filename.c_str());
				result &= false;
			}
		}

		size_t audioStreamIndex = STREAM_NOT_FOUND;
		size_t videoStreamIndex = STREAM_NOT_FOUND;

		if (result)
		{
			if (GetStreamIndices(audioStreamIndex, videoStreamIndex, desiredAudioStreamIndex) != true)
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
				m_audioContext = new AudioContext();
				if (!m_audioContext->Initialize(this, audioStreamIndex, audioOutputMode))
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
				m_videoContext = new VideoContext();
				if (!m_videoContext->Initialize(this, videoStreamIndex, outputMode))
					result &= false;
			}
		}

		return result;
	}

	bool Context::End()
	{
		Assert(m_begun == true);

		bool result = true;
		
		// Destroy audio context.
		if (m_audioContext)
		{
			Debug::Print("Destroying audio context.");

			result &= m_audioContext->Destroy();
			
			delete m_audioContext;
			m_audioContext = nullptr;
		}

		// Destroy video context.
		if (m_videoContext)
		{
			Debug::Print("Destroying video context.");

			result &= m_videoContext->Destroy();
			
			delete m_videoContext;
			m_videoContext = nullptr;
		}

		// Close file using AV codec.
		if (m_formatContext)
		{
			Debug::Print("Closing input file.");

			avformat_close_input(&m_formatContext);
			m_formatContext = nullptr;
		}
		
		m_filename.clear();
		m_eof = false;
		m_time = 0.0;
		
		m_begun = false;
		
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
	
	double Context::GetDuration() const
	{
		return m_formatContext->duration / double(AV_TIME_BASE);
	}

	size_t Context::GetVideoWidth() const
	{
		Assert(m_begun == true);

		if (m_videoContext)
			return m_videoContext->GetVideoWidth();
		else
			return 0;
	}

	size_t Context::GetVideoHeight() const
	{
		Assert(m_begun == true);

		if (m_videoContext)
			return m_videoContext->GetVideoHeight();
		else
			return 0;
	}
	
	double Context::GetVideoSampleAspectRatio() const
	{
		Assert(m_begun == true);

		if (m_videoContext)
		{
			const AVRational ratio = av_guess_sample_aspect_ratio(
				m_formatContext,
				m_formatContext->streams[m_videoContext->GetStreamIndex()],
				nullptr);
			
			if (ratio.num == 0 || ratio.den == 0)
				return 1.0;
			else
				return ratio.num / double(ratio.den);
		}
		else
			return 0;
	}

	size_t Context::GetAudioFrameRate() const
	{
		Assert(m_begun == true);

		if (m_audioContext)
			return m_audioContext->GetSampleRate();
		else
			return 0;
	}

	size_t Context::GetAudioChannelCount() const
	{
		Assert(m_begun == true);

		if (m_audioContext)
			return m_audioContext->GetOutputChannelCount();
		else
			return 0;
	}

	bool Context::RequestAudio(
		int16_t * __restrict out_samples, const size_t frameCount,
		bool   & out_gotAudio,
		double & out_audioTime)
	{
		Assert(m_begun == true);
		Assert(m_audioContext);

		bool result = true;

		if (m_audioContext == nullptr)
			result = false;
		else if (m_audioContext->RequestAudio(out_samples, frameCount, out_gotAudio, out_audioTime) != true)
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
		if (!HasReachedEOF()) // have all packets been read ?
			return false;
		if (m_audioContext && !m_audioContext->Depleted()) // have all packets been processed and their contents consumed ?
			return false;
		if (m_videoContext && !m_videoContext->Depleted()) // have all packets been processed and their contents consumed ?
			return false;
			
		return true;
	}

	void Context::FillAudioBuffer()
	{
		if (m_audioContext)
			m_audioContext->FillAudioBuffer();
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
			m_audioContext->ClearBuffers();
		}

		if (m_videoContext != nullptr)
		{
			streams.push_back(m_videoContext->GetStreamIndex());
			m_videoContext->ClearBuffers();
		}

		for (size_t i = 0; i < streams.size(); ++i)
		{
			if (av_seek_frame(
				m_formatContext,
				streams[i],
				0, // time stamp
				0*AVSEEK_FLAG_BACKWARD |
				0*AVSEEK_FLAG_BYTE |
				0*AVSEEK_FLAG_ANY) < 0)
			{
				result = false;
			}
		}

		return result;
	}

	bool Context::SeekToTime(const double time, const bool nearest, double & actualTime)
	{
		bool result = true;
		
		int streamIndex = -1;
		
		if (m_audioContext != nullptr)
		{
			streamIndex = m_audioContext->GetStreamIndex();
			m_audioContext->ClearBuffers();
		}

		if (m_videoContext != nullptr)
		{
			streamIndex = m_videoContext->GetStreamIndex();
			m_videoContext->ClearBuffers();
		}
		
	#if 1
		// note : streamIndex -1 works just fine, so we use it in combination with AV_TIME_BASE, instead of our own time stamp calculation (that may have
		//        unknown side effects that only become apparent in the future)
		streamIndex = -1;
	#endif
	
		// the time base is unique per stream, or AV_TIME_BASE for the special value of -1
		const int64_t timeStamp =
			streamIndex == -1
				? time * AV_TIME_BASE
				: time / av_q2d(m_formatContext->streams[streamIndex]->time_base);
	
		if (av_seek_frame(
			m_formatContext,
			streamIndex,
			timeStamp,
			1*AVSEEK_FLAG_BACKWARD |
			0*AVSEEK_FLAG_ANY) < 0)
		{
			result = false;
		}
		
		if (nearest == false)
		{
			// perform accurate seeking by first seeking before the given time stamp (above) and after that, decoding frames up until the requested time stamp (or the end of the stream is encountered)
			
			for (;;)
			{
				if (FillBuffers() == false)
					break;
				
				FillAudioBuffer();

				FillVideoBuffer();
		
				m_videoContext->GetVideoBuffer()->AdvanceToTime(time);
				
				VideoFrame * nextVideoFrame = m_videoContext->GetVideoBuffer()->PeekNextFrame();
				
				if (nextVideoFrame != nullptr && nextVideoFrame->m_time >= time)
					break;
				
				if (Depleted())
					break;
			}
		}
		else
		{
			// decode at least a single frame to find the actual seek time
			
			while (m_videoContext->GetVideoBuffer()->PeekNextFrame() == nullptr)
			{
				if (FillBuffers() == false)
					break;
				
				FillAudioBuffer();

				FillVideoBuffer();
				
				if (Depleted())
					break;
			}
		}
		
		//
		
		actualTime = 0.0;
		
		VideoFrame * nextVideoFrame = m_videoContext->GetVideoBuffer()->PeekNextFrame();
		
		if (nextVideoFrame != nullptr)
			actualTime = nextVideoFrame->m_time;
		
		return result;
	}

	AVFormatContext * Context::GetFormatContext()
	{
		return m_formatContext;
	}

	bool Context::GetStreamIndices(
		size_t & out_audioStreamIndex,
		size_t & out_videoStreamIndex,
		const int desiredAudioStreamIndex)
	{
		out_audioStreamIndex = STREAM_NOT_FOUND;
		out_videoStreamIndex = STREAM_NOT_FOUND;
		
		// Get stream info.
		if (avformat_find_stream_info(m_formatContext, nullptr) < 0)
		{
			Debug::Print("unable to get stream info");
			return false;
		}
		
		int audioStreamIndex = 0;

		// Find the first audio & video streams and use those during rendering.
		for (size_t i = 0; i < size_t(m_formatContext->nb_streams); ++i)
		{
			// Show stream info.
			av_dump_format(m_formatContext, i, m_filename.c_str(), false);
			
		#if LIBAVCODEC_VERSION_MAJOR >= 57
			if (m_formatContext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO)
			{
				if (out_audioStreamIndex == STREAM_NOT_FOUND || audioStreamIndex == desiredAudioStreamIndex)
				{
					out_audioStreamIndex = i;
				}
				
				audioStreamIndex++;
			}
		#else
			if (m_formatContext->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO)
			{
				if (out_audioStreamIndex == STREAM_NOT_FOUND || audioStreamIndex == desiredAudioStreamIndex)
				{
					out_audioStreamIndex = i;
				}
				
				audioStreamIndex++;
			}
		#endif

		#if LIBAVCODEC_VERSION_MAJOR >= 57
			if (out_videoStreamIndex == STREAM_NOT_FOUND)
			{
				if (m_formatContext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
				{
					out_videoStreamIndex = i;
				}
			}
		#else
			if (out_videoStreamIndex == STREAM_NOT_FOUND)
			{
				if (m_formatContext->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO)
				{
					out_videoStreamIndex = i;
				}
			}
		#endif
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
			
			// Packets from av_read_frame should always be unref'ed.
			av_packet_unref(&packet);
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
		
		Debug::Print("ProcessPacket: stream_index=%d.", packet.stream_index);

		if (m_audioContext != nullptr)
		{
			if (packet.stream_index == m_audioContext->GetStreamIndex())
			{
				result &= m_audioContext->AddPacket(packet);
			}
		}

		if (m_videoContext != nullptr)
		{
			if (packet.stream_index == m_videoContext->GetStreamIndex())
			{
				result &= m_videoContext->AddPacket(packet);
			}
		}

		return result;
	}

	bool Context::ReadPacket(AVPacket & out_packet)
	{
		// Read packet.
		av_init_packet(&out_packet);
		out_packet.buf = nullptr;
		out_packet.size = 0;
		
		const int readResult = av_read_frame(m_formatContext, &out_packet);

		if (readResult < 0)
			return false;

		if (out_packet.buf == 0)
		{
			Assert(0);
			return false;
		}

		return true;
	}
};
