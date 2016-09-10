#include "Debugging.h"
#include "MPContext.h"
#include "MPDebug.h"
#include "MPUtil.h"
#include "MPVideoContext.h"

#define QUEUE_SIZE (4 * 10)
//#define QUEUE_SIZE (4 * 30)
//#define QUEUE_SIZE (4)

namespace MP
{
	VideoContext::VideoContext()
		: m_packetQueue()
		, m_codecContext(nullptr)
		, m_codec(nullptr)
		, m_tempFrame(nullptr)
		, m_tempFrameBuffer(nullptr)
		, m_videoBuffer()
		, m_streamIndex(-1)
		, m_outputYuv(false)
		, m_time(0.0)
		, m_frameCount(0)
		, m_initialized(false)
	{
	}

	bool VideoContext::Initialize(Context * context, const size_t streamIndex, const bool outputYuv)
	{
		Assert(m_initialized == false);

		bool result = true;

		m_initialized = true;

		m_streamIndex = streamIndex;
		m_outputYuv = outputYuv;
		m_time = 0.0;
		m_frameCount = 0;

		// Get codec context for video stream.
		Assert(m_codecContext == nullptr);
		if (m_streamIndex != -1)
			m_codecContext = context->GetFormatContext()->streams[m_streamIndex]->codec;

		Util::SetDefaultCodecContextOptions(m_codecContext);

		// Get codec for video stream.
		Assert(m_codec == nullptr);
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
			if (avcodec_open(m_codecContext, m_codec) < 0)
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
				Assert(m_tempFrame == nullptr);
				m_tempFrame = avcodec_alloc_frame();

				if (!m_tempFrame)
				{
					return false;
				}
				else
				{
					// Allocate buffer to use for frame.
					const int frameBufferSize = avpicture_get_size(
						PIX_FMT_RGB24,
						m_codecContext->width,
						m_codecContext->height);

					Assert(m_tempFrameBuffer == nullptr);
					m_tempFrameBuffer = new uint8_t[frameBufferSize];

					// Assign buffer to frame.
					avpicture_fill(
						(AVPicture*)m_tempFrame,
						m_tempFrameBuffer,
						PIX_FMT_RGB24,
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
			m_tempFrame = nullptr;
		}

		if (m_tempFrameBuffer)
		{
			delete [] m_tempFrameBuffer;
			m_tempFrameBuffer = nullptr;
		}

		// Close video codec context.
		if (m_codecContext != nullptr)
			avcodec_close(m_codecContext);

		return result;
	}

	size_t VideoContext::GetStreamIndex() const
	{
		return m_streamIndex;
	}

	double VideoContext::GetTime() const
	{
		return m_time;
	}

	void VideoContext::FillVideoBuffer()
	{
		while (!m_videoBuffer.IsFull() && !m_packetQueue.IsEmpty() && ProcessPacket(m_packetQueue.GetPacket()))
			m_packetQueue.PopFront();
	}

	bool VideoContext::RequestVideo(const double time, VideoFrame ** out_frame, bool & out_gotVideo)
	{
		Assert(out_frame);

		bool result = true;

		out_gotVideo = false;

		// Check if the frame is in the buffer.
		VideoFrame * oldFrame = m_videoBuffer.GetCurrentFrame();
		m_videoBuffer.AdvanceToTime(time);
		VideoFrame * newFrame = m_videoBuffer.GetCurrentFrame();

		*out_frame = newFrame;

		if (newFrame && newFrame != oldFrame)
		{
			out_gotVideo = true;
		}

		return result;
	}

	bool VideoContext::IsQueueFull() const
	{
		return m_packetQueue.GetSize() >= QUEUE_SIZE;
	}

	bool VideoContext::AddPacket(const AVPacket & packet)
	{
		m_packetQueue.PushBack(packet);

		Debug::Print("PQVIDEO");

		return true;
	}

	bool VideoContext::ProcessPacket(AVPacket & packet)
	{
		int       bytesRemaining = 0;
		uint8_t * packetData     = nullptr;
		int       bytesDecoded   = 0;
		int       frameFinished  = 0;

		Assert(packet.data);
		Assert(packet.size >= 0);

		bytesRemaining = packet.size;
		packetData = packet.data;

		// Decode entire packet.
		while (bytesRemaining > 0)
		{
			// Decode some data.
			bytesDecoded = avcodec_decode_video(
				m_codecContext,
				m_tempFrame,
				&frameFinished,
				packetData,
				bytesRemaining);

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
					VideoFrame * frame = m_videoBuffer.AllocateFrame();

					Convert(frame);

					m_videoBuffer.StoreFrame(frame);
				}
			}
		}

		// Free current packet.
		if (packet.data)
			av_free_packet(&packet);

		return true;
	}

	bool VideoContext::AdvanceToTime(const double time, VideoFrame ** out_currentFrame)
	{
		m_videoBuffer.AdvanceToTime(time);

		*out_currentFrame = m_videoBuffer.GetCurrentFrame();

		return true;
	}

	bool VideoContext::Depleted() const
	{
		return m_videoBuffer.Depleted() && (m_packetQueue.GetSize() == 0);
	}

	bool VideoContext::Convert(VideoFrame * out_frame)
	{
		bool result = true;

		const AVPicture * src = (const AVPicture*)m_tempFrame;
		      AVPicture * dst = (AVPicture*)out_frame->m_frame;

		if (m_outputYuv)
		{
			const uint8_t * __restrict srcYLine = src->data[0];
			const uint8_t * __restrict srcULine = src->data[1];
			const uint8_t * __restrict srcVLine = src->data[2];
			uint8_t * __restrict dstLine  = dst->data[0];

			const int sx = m_codecContext->width;
			const int sy = m_codecContext->height;

			for (int y = 0; y < sy; ++y)
			{
				const uint8_t * __restrict srcY   = srcYLine + src->linesize[0] * (y     );
				const uint8_t * __restrict srcU   = srcULine + src->linesize[1] * (y >> 1);
				const uint8_t * __restrict srcV   = srcVLine + src->linesize[2] * (y >> 1);
				uint8_t * __restrict dstRGB = dstLine  + dst->linesize[0] * (y     );

				for (int x = 0; x < sx; ++x, dstRGB += 3)
				{
					const int y = srcY[x];
					const int u = srcU[x >> 1];
					const int v = srcV[x >> 1];

					dstRGB[0] = y;
					dstRGB[1] = u;
					dstRGB[2] = v;
				}
			}
		}
		else
		{
			img_convert((AVPicture *)out_frame->m_frame, PIX_FMT_RGB24, src, 
				m_codecContext->pix_fmt,
				m_codecContext->width,
				m_codecContext->height);
		}

        if (m_tempFrame->pts != 0 && m_tempFrame->pts != AV_NOPTS_VALUE)
		{
			m_time = av_q2d(m_codecContext->time_base) * m_tempFrame->pts;
		}
		else
		{
			double frameDelay = av_q2d(m_codecContext->time_base);
			frameDelay += m_tempFrame->repeat_pict * (frameDelay * 0.5);

			m_time += frameDelay;
		}

		out_frame->m_time = m_time;

		++m_frameCount;

		Debug::Print("VIDEO: Decoded frame. Time = %03.3f.", m_time);

		return result;
	}
};
