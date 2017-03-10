#include "Debugging.h"
#include "MPContext.h"
#include "MPDebug.h"
#include "MPPacketQueue.h"
#include "MPUtil.h"
#include "MPVideoBuffer.h"
#include "MPVideoContext.h"
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>

#define QUEUE_SIZE (4 * 10)
//#define QUEUE_SIZE (4 * 30)
//#define QUEUE_SIZE (4)

namespace MP
{
	VideoContext::VideoContext()
		: m_packetQueue(nullptr)
		, m_codecContext(nullptr)
		, m_codec(nullptr)
		, m_tempFrame(nullptr)
		, m_tempFrameBuffer(nullptr)
		, m_videoBuffer(nullptr)
		, m_swsContext(nullptr)
		, m_streamIndex(-1)
		, m_outputYuv(false)
		, m_time(0.0)
		, m_frameCount(0)
		, m_initialized(false)
	{
	}
	
	VideoContext::~VideoContext()
	{
		Assert(m_initialized == false);
	}

	bool VideoContext::Initialize(Context * context, const size_t streamIndex, const bool outputYuv)
	{
		Assert(m_initialized == false);
		
		m_initialized = true;

		m_streamIndex = streamIndex;
		m_outputYuv = outputYuv;
		m_time = 0.0;
		m_frameCount = 0;
		
		Assert(m_packetQueue == nullptr);
		m_packetQueue = new PacketQueue();
		
		Assert(m_videoBuffer == nullptr);
		m_videoBuffer = new VideoBuffer();

		AVCodecParameters * codecParams = context->GetFormatContext()->streams[m_streamIndex]->codecpar;
		if (!codecParams)
		{
			Debug::Print("Video: codec params missing.");
			return false;
		}
		
		// Get codec for video stream.
		Assert(m_codec == nullptr);
		m_codec = avcodec_find_decoder(codecParams->codec_id);
		if (!m_codec)
		{
			Assert(false);
			return false;
		}
		else
		{
			Debug::Print("Video: codec: %s.", m_codec->name);
			
			// Get codec context for video stream.
			Assert(m_codecContext == nullptr);
			m_codecContext = avcodec_alloc_context3(m_codec);
			if (!m_codecContext)
			{
				Debug::Print("Video: failed to allocate codec context.");
				return false;
			}
			
			if (avcodec_parameters_to_context(m_codecContext, codecParams) < 0)
			{
				Debug::Print("Video: failed to set codec params on codec context.");
			}
			
			// Open codec.
			if (avcodec_open2(m_codecContext, m_codec, nullptr) < 0)
			{
				Debug::Print("Video: failed to open codec context.");
				return false;
			}
			else
			{
				// Display codec info.
				Debug::Print("Video: width: %d.", m_codecContext->width);
				Debug::Print("Video: height: %d.", m_codecContext->height);
				Debug::Print("Video: bitrate: %d.", m_codecContext->bit_rate);

				// Create frame.
				Assert(m_tempFrame == nullptr);
				m_tempFrame = av_frame_alloc();

				if (!m_tempFrame)
				{
					Debug::Print("Video: failed to allocate AV frame for decode.");
					return false;
				}
				else
				{
					// Allocate buffer to use for frame.
					const int frameBufferSize = av_image_get_buffer_size(
						m_codecContext->pix_fmt,
						m_codecContext->width,
						m_codecContext->height,
						1);

					Assert(m_tempFrameBuffer == nullptr);
					m_tempFrameBuffer = (uint8_t*)_mm_malloc(frameBufferSize, 16);

					// Assign buffer to frame.
					const int requiredFrameBufferSize = av_image_fill_arrays(
						m_tempFrame->data, m_tempFrame->linesize, m_tempFrameBuffer,
						m_codecContext->pix_fmt, m_codecContext->width, m_codecContext->height, 16);
					
					Debug::Print("Vide: frameBufferSize: %d.", frameBufferSize);
					Debug::Print("Video: requiredFrameBufferSize: %d.", requiredFrameBufferSize);
					
					if (requiredFrameBufferSize > frameBufferSize)
					{
						Debug::Print("Video: required frame buffer size exceeds allocated frame buffer size.");
						return false;
					}
					
					if (!m_videoBuffer->Initialize(m_codecContext->width, m_codecContext->height))
					{
						Debug::Print("Video: failed to initialize video buffer.");
						return false;
					}

					m_swsContext = sws_getContext(
						m_codecContext->width, m_codecContext->height, m_codecContext->pix_fmt,
						m_codecContext->width, m_codecContext->height, AV_PIX_FMT_RGBA,
						SWS_POINT, nullptr, nullptr, nullptr);
					
					if (!m_swsContext)
					{
						Debug::Print("Video: failed to allocated sws context.");
						return false;
					}
					
					m_timeBase = av_q2d(context->GetFormatContext()->streams[streamIndex]->time_base);
					
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

		if (m_swsContext)
		{
			sws_freeContext(m_swsContext);
			m_swsContext = nullptr;
		}
		
		if (m_videoBuffer != nullptr)
		{
			if (m_videoBuffer->IsInitialized())
			{
				m_videoBuffer->Destroy();
			}
			
			delete m_videoBuffer;
			m_videoBuffer = nullptr;
		}

		if (m_tempFrame != nullptr)
		{
			av_frame_unref(m_tempFrame);
			m_tempFrame = nullptr;
		}

		if (m_tempFrameBuffer != nullptr)
		{
			_mm_free(m_tempFrameBuffer);
			m_tempFrameBuffer = nullptr;
		}

		// Close video codec context.
		if (m_codecContext != nullptr)
		{
			avcodec_free_context(&m_codecContext);
			m_codecContext = nullptr;
		}
		
		if (m_codec != nullptr)
		{
			m_codec = nullptr;
		}
		
		if (m_packetQueue != nullptr)
		{
			delete m_packetQueue;
			m_packetQueue = nullptr;
		}

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
		while (!m_videoBuffer->IsFull() && !m_packetQueue->IsEmpty() && ProcessPacket(m_packetQueue->GetPacket()))
			m_packetQueue->PopFront();
	}

	bool VideoContext::RequestVideo(const double time, VideoFrame ** out_frame, bool & out_gotVideo)
	{
		Assert(out_frame);

		bool result = true;

		out_gotVideo = false;

		// Check if the frame is in the buffer.
		VideoFrame * oldFrame = m_videoBuffer->GetCurrentFrame();
		m_videoBuffer->AdvanceToTime(time);
		VideoFrame * newFrame = m_videoBuffer->GetCurrentFrame();

		*out_frame = newFrame;

		if (newFrame && newFrame != oldFrame)
		{
			out_gotVideo = true;
		}

		return result;
	}

	bool VideoContext::IsQueueFull() const
	{
		return m_packetQueue->GetSize() >= QUEUE_SIZE;
	}

	bool VideoContext::AddPacket(const AVPacket & packet)
	{
		m_packetQueue->PushBack(packet);

		Debug::Print("PQVIDEO");

		return true;
	}

	bool VideoContext::ProcessPacket(AVPacket & _packet)
	{
		AVPacket packet = _packet;
		
		Assert(packet.data);
		Assert(packet.size >= 0);

		packet.data = packet.buf->data;
		int bytesRemaining = packet.size;
		
		// Decode entire packet.
		while (bytesRemaining > 0)
		{
			int gotPicture = 0;
			
			// Decode some data.
			const int bytesDecoded = avcodec_decode_video2(
				m_codecContext,
				m_tempFrame,
				&gotPicture,
				&packet);

			if (bytesDecoded < 0)
			{
				Debug::Print("Video: unable to decode video packet.");
				bytesRemaining = 0;
				Assert(0);
			}
			else
			{
				bytesRemaining -= bytesDecoded;
				packet.data += bytesDecoded;
				packet.size -= bytesDecoded;

				Assert(bytesDecoded >= 0);
				Assert(bytesRemaining >= 0);

				// Video frame finished?
				if (gotPicture)
				{
					VideoFrame * frame = m_videoBuffer->AllocateFrame();

					Convert(frame);

					m_videoBuffer->StoreFrame(frame);
				}
			}
		}
		
		return true;
	}

	bool VideoContext::AdvanceToTime(const double time, VideoFrame ** out_currentFrame)
	{
		m_videoBuffer->AdvanceToTime(time);

		*out_currentFrame = m_videoBuffer->GetCurrentFrame();

		return true;
	}

	bool VideoContext::Depleted() const
	{
		return m_videoBuffer->Depleted() && (m_packetQueue->GetSize() == 0);
	}

	bool VideoContext::Convert(VideoFrame * out_frame)
	{
		bool result = true;
		
		const AVFrame & src = *m_tempFrame;
		      AVFrame & dst = *out_frame->m_frame;

		if (m_outputYuv)
		{
			const uint8_t * __restrict srcYLine = src.data[0];
			const uint8_t * __restrict srcULine = src.data[1];
			const uint8_t * __restrict srcVLine = src.data[2];
			uint8_t * __restrict dstLine  = dst.data[0];

			const int sx = m_codecContext->width;
			const int sy = m_codecContext->height;

			for (int y = 0; y < sy; ++y)
			{
				const uint8_t * __restrict srcY    = srcYLine + src.linesize[0] * (y     );
				const uint8_t * __restrict srcU    = srcULine + src.linesize[1] * (y >> 1);
				const uint8_t * __restrict srcV    = srcVLine + src.linesize[2] * (y >> 1);
				      uint8_t * __restrict dstRGBA = dstLine  + dst.linesize[0] * (y     );

				for (int x = 0; x < sx; ++x, dstRGBA += 4)
				{
					const uint8_t y = srcY[x];
					const uint8_t u = srcU[x >> 1];
					const uint8_t v = srcV[x >> 1];

					dstRGBA[0] = y;
					dstRGBA[1] = u;
					dstRGBA[2] = v;
					dstRGBA[3] = 255;
				}
			}
		}
		else
		{
			sws_scale(m_swsContext, src.data, src.linesize, 0, src.height, dst.data, dst.linesize);
		}
		
        //if (m_tempFrame->pts != 0 && m_tempFrame->pts != AV_NOPTS_VALUE)
		if (false)
		{
			m_time = av_frame_get_best_effort_timestamp(m_tempFrame) * m_timeBase;
		}
		else
		{
			double frameDelay = av_q2d(m_codecContext->time_base);
			frameDelay += m_tempFrame->repeat_pict * (frameDelay * 0.5);

			m_time += frameDelay;
		}
		
		out_frame->m_time = m_time;
		out_frame->m_isFirstFrame = (m_frameCount == 0);

		++m_frameCount;

		Debug::Print("Video: decoded frame. time: %03.3f.", float(m_time));

		return result;
	}
};
