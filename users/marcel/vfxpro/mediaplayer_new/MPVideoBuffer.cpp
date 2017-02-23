#include "Debugging.h"
#include "MPDebug.h"
#include "MPVideoBuffer.h"

//#define BUFFER_SIZE (3)
#define BUFFER_SIZE (10)
//#define BUFFER_SIZE (60)

namespace MP
{
	// Mutex

	Mutex::Mutex()
		: mutex(nullptr)
	{
		mutex = SDL_CreateMutex();
	}

	Mutex::~Mutex()
	{
		SDL_DestroyMutex(mutex);
		mutex = nullptr;
	}

	void Mutex::Lock()
	{
		SDL_LockMutex(mutex);
	}

	void Mutex::Unlock()
	{
		SDL_UnlockMutex(mutex);
	}

	//

	// VideoFrame
	VideoFrame::VideoFrame()
		: m_width(0)
		, m_height(0)
		, m_frame(nullptr)
		, m_frameBuffer(nullptr)
		, m_time(0.0)
		, m_initialized(false)
	{
	}

	VideoFrame::~VideoFrame()
	{
		Assert(m_initialized == false);
	}

	void VideoFrame::Initialize(const size_t width, const size_t height)
	{
		Assert(m_initialized == false);

		m_initialized = true;

		m_width = width;
		m_height = height;

		// Create frames.
		m_frame = av_frame_alloc();

		if (!m_frame)
		{
			Assert(0);
			return;
		}

	/*
		// Allocate buffer to use for RGB frame.
		const int frameBufferSize = avpicture_get_size(
			PIX_FMT_RGB24,
			static_cast<int>(width),
			static_cast<int>(height));
		
		m_frameBuffer = new uint8_t[frameBufferSize];

		// Assign buffer to frame.
		avpicture_fill(
			(AVPicture*)m_frame,
			m_frameBuffer,
			PIX_FMT_RGB24,
			static_cast<int>(width),
			static_cast<int>(height));
	 */
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
			delete [] m_frameBuffer;
			m_frameBuffer = nullptr;
		}
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
	}

	bool VideoBuffer::Initialize(const size_t width, const size_t height)
	{
		Assert(m_initialized == false);

		bool result = true;

		m_initialized = true;

		for (int i = 0; i < BUFFER_SIZE; ++i)
		{
			VideoFrame * frame = new VideoFrame();

			frame->Initialize(width, height);

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

		for (auto frame : m_freeList)
		{
			frame->Destroy();

			delete frame;
			frame = nullptr;
		}

		m_freeList.clear();

		return result;
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

			//memset(frame->m_frameBuffer, 0xcc, frame->m_width * frame->m_height * 3);
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

		while (!m_consumeList.empty() && m_consumeList.front()->m_time < time)
		{
			if (m_currentFrame != nullptr)
			{
				m_freeList.push_back(m_currentFrame);
				m_currentFrame = nullptr;
			}

			m_currentFrame = m_consumeList.front();
			m_consumeList.pop_front();

			++skipCount;

			Debug::Print("Advancing frame.");
		}

		m_mutex.Unlock();

		if (skipCount > 1)
		{
			Debug::Print("VIDEO: Warning: Skipped %d frames.", skipCount - 1);
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
			m_freeList.push_back(m_currentFrame);
			m_currentFrame = nullptr;
		}

		for (auto frame : m_consumeList)
			m_freeList.push_back(frame);
		m_consumeList.clear();

		m_currentFrame = nullptr;
	}
};
