#include "Debugging.h"
#include "MPDebug.h"
#include "MPVideoBuffer.h"
#include <libavcodec/avcodec.h>
#include <libavutil/imgutils.h>

#if DEBUG_MEDIAPLAYER
#include <atomic>
static std::atomic_int s_numFrameBufferAllocations(0);
#endif

//#define BUFFER_SIZE (3)
#define BUFFER_SIZE (10)
//#define BUFFER_SIZE (60)

namespace MP
{
	// VideoFrame
	VideoFrame::VideoFrame()
		: m_width(0)
		, m_height(0)
		, m_pixelFormat(AV_PIX_FMT_NONE)
		, m_frame(nullptr)
		, m_frameBuffer(nullptr)
		, m_frameBufferSize(0)
		, m_time(0.0)
		, m_isFirstFrame(false)
		, m_isValidForRead(false)
		, m_initialized(false)
	{
	}

	VideoFrame::~VideoFrame()
	{
		Assert(m_initialized == false);
		
		Assert(m_width == 0);
		Assert(m_height == 0);
		Assert(m_pixelFormat == AV_PIX_FMT_NONE);
		Assert(m_frame == nullptr);
		Assert(m_frameBuffer == nullptr);
		Assert(m_frameBufferSize == 0);
		Assert(m_time == 0.0);
		Assert(m_isFirstFrame == false);
		Assert(m_isValidForRead == false);
	}

	bool VideoFrame::Initialize(const size_t width, const size_t height, const size_t _pixelFormat)
	{
		Assert(m_initialized == false);
		
		const AVPixelFormat pixelFormat = (AVPixelFormat)_pixelFormat;

		m_initialized = true;

		m_width = width;
		m_height = height;
		m_pixelFormat = pixelFormat;

		// Create frames.
		m_frame = av_frame_alloc();

		if (!m_frame)
		{
			Debug::Print("Video: frame allocation failed");
			return false;
		}
	
		// Allocate buffer to use for RGB frame.
		m_frameBufferSize = av_image_get_buffer_size(
			pixelFormat,
			static_cast<int>(width),
			static_cast<int>(height),
			16);
		
		m_frameBuffer = (uint8_t*)_mm_malloc(m_frameBufferSize, 16);
		
		if (!m_frameBuffer)
		{
			Debug::Print("Video: failed to allocate frame buffer.");
			m_frameBuffer = nullptr;
			return false;
		}
		
	#if DEBUG_MEDIAPLAYER
		s_numFrameBufferAllocations++;
	#endif
		
		m_frame->format = pixelFormat;
		m_frame->width = width;
		m_frame->height = height;
		const int requiredFrameBufferSize = av_image_fill_arrays(m_frame->data, m_frame->linesize, m_frameBuffer, pixelFormat, width, height, 16);
		
		Debug::Print("Video: frameBufferSize: %d.", m_frameBufferSize);
		Debug::Print("Video: requiredFrameBufferSize: %d.", requiredFrameBufferSize);
		
		if (requiredFrameBufferSize > m_frameBufferSize)
		{
			Debug::Print("Video: required frame buffer size exceeds allocated frame buffer size.");
			return false;
		}
		
		return true;
	}

	void VideoFrame::Destroy()
	{
		Assert(m_initialized == true);

		m_initialized = false;

		if (m_frame)
		{
			av_frame_unref(m_frame);
			m_frame = nullptr;
		}

		if (m_frameBuffer)
		{
			_mm_free(m_frameBuffer);
			m_frameBuffer = nullptr;
			m_frameBufferSize = 0;
			
		#if DEBUG_MEDIAPLAYER
			s_numFrameBufferAllocations--;
			//printf("numFrameBufferAllocations: %d\n", (int)s_numFrameBufferAllocations);
		#endif
		}
		
		m_width = 0;
		m_height = 0;
		m_pixelFormat = AV_PIX_FMT_NONE;
		m_time = 0.0;
		m_isFirstFrame = false;
		m_isValidForRead = false;
	}
	
	uint8_t * VideoFrame::getY(int & sx, int & sy, int & pitch) const
	{
		Assert(m_isValidForRead && m_pixelFormat == AV_PIX_FMT_YUV420P);
		
		sx = m_frame->width;
		sy = m_frame->height;
		pitch = m_frame->linesize[0];
		
		return m_frame->data[0];
	}
	
	uint8_t * VideoFrame::getU(int & sx, int & sy, int & pitch) const
	{
		Assert(m_isValidForRead && m_pixelFormat == AV_PIX_FMT_YUV420P);
		
		sx = m_frame->width / 2;
		sy = m_frame->height / 2;
		pitch = m_frame->linesize[1];
		
		return m_frame->data[1];
	}
	
	uint8_t * VideoFrame::getV(int & sx, int & sy, int & pitch) const
	{
		Assert(m_isValidForRead && m_pixelFormat == AV_PIX_FMT_YUV420P);
		
		sx = m_frame->width / 2;
		sy = m_frame->height / 2;
		pitch = m_frame->linesize[2];
		
		return m_frame->data[2];
	}

	// VideBuffer
	VideoBuffer::VideoBuffer()
		: m_freeList()
		, m_consumeList()
		, m_currentFrame(nullptr)
		, m_initialized(false)
	{
	}

	VideoBuffer::~VideoBuffer()
	{
		Assert(m_initialized == false);
		
		Assert(m_freeList.empty());
		Assert(m_consumeList.empty());
		Assert(m_currentFrame == nullptr);
	}

	bool VideoBuffer::Initialize(const size_t width, const size_t height, const size_t pixelFormat)
	{
		Assert(m_initialized == false);

		bool result = true;

		m_initialized = true;

		for (int i = 0; i < BUFFER_SIZE; ++i)
		{
			VideoFrame * frame = new VideoFrame();

			result &= frame->Initialize(width, height, pixelFormat);

			m_freeList.push_back(frame);
		}

		m_currentFrame = nullptr;

		return result;
	}

	bool VideoBuffer::Destroy()
	{
		Assert(m_initialized == true);

		bool result = true;

		m_initialized = false;

		Clear();
		
		Assert(m_freeList.size() == BUFFER_SIZE);
		Assert(m_consumeList.empty());
		
		for (auto frame : m_freeList)
		{
			frame->Destroy();

			delete frame;
			frame = nullptr;
		}

		m_freeList.clear();

		return result;
	}
	
	bool VideoBuffer::IsInitialized() const
	{
		return m_initialized;
	}

	VideoFrame * VideoBuffer::AllocateFrame()
	{
		Assert(!IsFull());

		if (IsFull())
		{
			return nullptr;
		}

		VideoFrame * frame = nullptr;

		m_mutex.Lock();
		{
			frame = m_freeList.front();
			m_freeList.pop_front();

			Assert(frame != m_currentFrame);
			
			//memset(frame->m_frameBuffer, 0xcc, frame->m_frameBufferSize);
		}
		m_mutex.Unlock();

		return frame;
	}

	void VideoBuffer::StoreFrame(VideoFrame * frame)
	{
		m_mutex.Lock();
		{
			m_consumeList.push_back(frame);
		}
		m_mutex.Unlock();
	}

	VideoFrame * VideoBuffer::GetCurrentFrame()
	{
		return m_currentFrame;
	}

	void VideoBuffer::AdvanceToTime(double time)
	{
		m_mutex.Lock();

		// Skip as many frame necessary to reach the specified time.
		// Stop moving forward until the current write position (last written frame) is reached.

		int skipCount = 0;

		while (!m_consumeList.empty() && (m_consumeList.front()->m_time < time || m_consumeList.front()->m_isFirstFrame))
		{
			if (m_currentFrame != nullptr)
			{
				Assert(m_currentFrame != m_consumeList.front());
				Assert(m_currentFrame->m_isValidForRead);
				m_currentFrame->m_isValidForRead = false;
				m_freeList.push_back(m_currentFrame);
				m_currentFrame = nullptr;
			}

			m_currentFrame = m_consumeList.front();
			m_consumeList.pop_front();
			
			Assert(!m_currentFrame->m_isValidForRead);
			m_currentFrame->m_isValidForRead = true;

			++skipCount;

			Debug::Print("Video: Advancing frame.");
		}

		m_mutex.Unlock();

		if (skipCount > 1)
		{
			Debug::Print("Video: Warning: Skipped %d frames.", skipCount - 1);
		}
	}

	bool VideoBuffer::Depleted() const
	{
		return m_consumeList.empty();
	}

	bool VideoBuffer::IsFull() const
	{
		return m_freeList.empty();
	}

	void VideoBuffer::Clear()
	{
		if (m_currentFrame != nullptr)
		{
			Assert(m_currentFrame->m_isValidForRead);
			m_currentFrame->m_isValidForRead = false;
			
			m_freeList.push_back(m_currentFrame);
			m_currentFrame = nullptr;
		}

		for (auto frame : m_consumeList)
			m_freeList.push_back(frame);
		m_consumeList.clear();
	}
};
