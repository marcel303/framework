#include "Debugging.h"
#include "MPContext.h"
#include "MPDebug.h"
#include "MPUtil.h"
#include "MPVideoContext.h"

//#define QUEUE_SIZE (4 * 10)
#define QUEUE_SIZE (4 * 30)
//#define QUEUE_SIZE (4)

namespace MP
{
	VideoContext::VideoContext()
	{
		m_initialized = false;

		m_codecContext = 0;
		m_codec = 0;
	}

	bool VideoContext::Initialize(Context* context, size_t streamIndex)
	{
		Assert(m_initialized == false);

		bool result = true;

		m_initialized = true;

		m_streamIndex = streamIndex;
		m_time = 0.0;
		m_frameCount = 0;

		// Get codec context for video stream.
		if (m_streamIndex != -1)
			m_codecContext = context->GetFormatContext()->streams[m_streamIndex]->codec;

		Util::SetDefaultCodecContextOptions(m_codecContext);

		// Get codec for video stream.
		m_codec = avcodec_find_decoder(m_codecContext->codec_id);
		if (!m_codec)
		{
			Assert(false);
			return false;
		}
		else
		{
			Debug::Print("Video codec: %s.", m_codec->name);

			// Open codec.
			if (avcodec_open2(m_codecContext, m_codec, nullptr) < 0)
			{
				return false;
			}
			else
			{
				// Display codec info.
				Debug::Print("Video: width: %d.",   m_codecContext->width   );
				Debug::Print("Video: height: %d.",  m_codecContext->height  );
				Debug::Print("Video: bitrate: %d.", m_codecContext->bit_rate);

				// Create frame.
				m_tempFrame = av_frame_alloc();

				if (!m_tempFrame)
				{
					return false;
				}
				else
				{
					// Allocate buffer to use for frame.
					int frameBufferSize = avpicture_get_size(
						AV_PIX_FMT_RGB24,
						m_codecContext->width,
						m_codecContext->height);

					m_tempFrameBuffer = new uint8_t[frameBufferSize];

					// Assign buffer to frame.
					avpicture_fill(
						(AVPicture*)m_tempFrame,
						m_tempFrameBuffer,
						AV_PIX_FMT_RGB24,
						m_codecContext->width,
						m_codecContext->height);

					m_videoBuffer.Initialize(m_codecContext->width, m_codecContext->height);

					return true;
				}
			}
		}
	}

	bool VideoContext::Destroy()
	{
		Assert(m_initialized == true);

		bool result = true;

		m_initialized = false;

		if (m_videoBuffer.m_initialized)
			m_videoBuffer.Destroy();

		if (m_tempFrame)
		{
			av_free(m_tempFrame);
			m_tempFrame = 0;
		}

		if (m_tempFrameBuffer)
		{
			delete[] m_tempFrameBuffer;
			m_tempFrameBuffer = 0;
		}

		// Close video codec context.
		if (m_codecContext)
			avcodec_close(m_codecContext);

		return result;
	}

	size_t VideoContext::GetStreamIndex()
	{
		return m_streamIndex;
	}

	double VideoContext::GetTime()
	{
		return m_time;
	}

	bool VideoContext::RequestVideo(double time, VideoFrame** out_frame, bool& out_gotVideo)
	{
		Assert(out_frame);

		bool result = true;

		out_gotVideo = false;
		bool stop = false;

		// Check if the frame is in the buffer.
		VideoFrame* oldFrame = m_videoBuffer.GetCurrentFrame();
		m_videoBuffer.AdvanceToTime(time);
		VideoFrame* newFrame = m_videoBuffer.GetCurrentFrame();

		static bool first = true;

		if (newFrame != oldFrame)
		{
			*out_frame = newFrame;
			Debug::Print("Got buffered video.");
			if (first == true)
				first = false;
			else
				out_gotVideo = true;
			return true;
		}

		// FIXME: Uncomment.
#if 1
		if (newFrame->m_time >= time)
		{
			//Debug::Print("No new video frame. Time = %f vs %f.", time, newFrame->m_time);
			return true;
		}
#endif

		while (m_packetQueue.GetSize() > 0 && stop == false)
		{
			AVPacket& packet = m_packetQueue.GetPacket();

			bool newFrame;

			if (ProcessPacket(packet, newFrame) != true)
				result = false;

			if (newFrame)
			{
				if (AdvanceToTime(time, out_frame) != true)
					result = false;

				if ((*out_frame)/* && (*out_frame)->m_time >= time*/)
				{
					Debug::Print("VIDEO: Request frame. Time = %f (requested = %f).", (*out_frame)->m_time, time);
					out_gotVideo = true;
					stop = true;
				}

				/*
				if (time > (*out_frame)->m_time)
				{
					Debug::Print("Decoded skip frame.");
					stop = true;
				}
				*/
			}

			m_packetQueue.PopFront();
		}

		return result;
	}

	bool VideoContext::IsQueueFull()
	{
		return m_packetQueue.GetSize() > QUEUE_SIZE;
	}

	bool VideoContext::AddPacket(AVPacket& packet)
	{
		m_packetQueue.PushBack(packet);

		Debug::Print("PQVIDEO");

		return true;
	}

	bool VideoContext::ProcessPacket(AVPacket& packet, bool& out_newFrame)
	{
		out_newFrame = false;

		int      bytesRemaining = 0;
		uint8_t* packetData     = 0;
		int      bytesDecoded   = 0;
		int      frameFinished  = 0;

		Assert(packet.data);
		Assert(packet.size >= 0);

		bytesRemaining = packet.size;
		packetData = packet.data;

		// Decode entire packet.
		while (bytesRemaining > 0)
		{
			AVPacket packet2 = packet;
			packet2.size = bytesRemaining;
			packet2.data = packetData;

			// Decode some data.
			bytesDecoded = avcodec_decode_video2(
				m_codecContext,
				m_tempFrame,
				&frameFinished,
				&packet2);
				//packetData,
				//bytesRemaining);

			if (bytesDecoded < 0)
			{
				Debug::Print("Unable to decode video packet.");
				bytesRemaining = 0;
				Assert(0);
			}
			else
			{
				bytesRemaining -= bytesDecoded;
				packetData += bytesDecoded;

				Assert(bytesDecoded >= 0);
				Assert(bytesRemaining >= 0);

				// Video frame finished?
				if (frameFinished)
				{
					VideoFrame* frame = m_videoBuffer.AllocateFrame();

					if (m_tempFrame->pts == 0)
						m_tempFrame->pts = packet.pts;

					ConvertAndStore(frame);

					out_newFrame = true;
				}

			}

		}

		// Free current packet.
		if (packet.data)
			av_free_packet(&packet);

		return true;
	}

	bool VideoContext::AdvanceToTime(double time, VideoFrame** out_currentFrame)
	{
		m_videoBuffer.AdvanceToTime(time);

		*out_currentFrame = m_videoBuffer.GetCurrentFrame();

		return true;
	}

	bool VideoContext::ConvertAndStore(VideoFrame* out_frame)
	{
		bool result = true;

		img_convert((AVPicture *)out_frame->m_frame, AV_PIX_FMT_RGB24, (AVPicture*)m_tempFrame, 
			m_codecContext->pix_fmt,
			m_codecContext->width,
			m_codecContext->height);

        if (m_tempFrame->pts != AV_NOPTS_VALUE)
		{
			// m_tempFrame->pts * 1345.2944 << Magic number.
			m_time = av_q2d(m_codecContext->time_base) * m_tempFrame->pts;
		}
		else
		{
			double frame_delay = av_q2d(m_codecContext->time_base);
			frame_delay += m_tempFrame->repeat_pict * (frame_delay * 0.5f);

			m_time += frame_delay;
		}

		out_frame->m_time = m_time;

		++m_frameCount;

		Debug::Print("VIDEO: Decoded frame. Time = %03.3f.", m_time);

		return result;
	}
};