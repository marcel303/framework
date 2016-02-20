#include "Debugging.h"
#include "MPDebug.h"
#include "MPVideoBuffer.h"

//#define BUFFER_SIZE (3)
#define BUFFER_SIZE (10)

namespace MP
{
	// VideoFrame
	VideoFrame::VideoFrame()
	{
		m_frame = 0;
		m_frameBuffer = 0;

		m_initialized = false;
	}

	VideoFrame::~VideoFrame()
	{
		Assert(m_frame == 0);
	}

	void VideoFrame::Initialize(size_t width, size_t height)
	{
		Assert(m_initialized == false);

		m_initialized = true;

		m_width = width;
		m_height = height;

		// Create frames.
		m_frame = avcodec_alloc_frame();

		if (!m_frame)
		{
			Assert(0);
			return;
		}

		// Allocate buffer to use for RGB frame.
		int frameBufferSize = avpicture_get_size(
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
	}

	void VideoFrame::Destroy()
	{
		Assert(m_initialized == true);

		m_initialized = false;

		if (m_frame)
		{
			av_free(m_frame);
			m_frame = 0;
		}

		if (m_frameBuffer)
		{
			delete[] m_frameBuffer;
			m_frameBuffer = 0;
		}
	}

	// VideBuffer
	VideoBuffer::VideoBuffer()
	{
		m_initialized = false;
	}

	bool VideoBuffer::Initialize(size_t width, size_t height)
	{
		Assert(m_initialized == false);

		bool result = true;

		m_initialized = true;

		m_buffer.setSize(BUFFER_SIZE);

		m_buffer[0].m_time = 0.0;

		for (size_t i = 0; i < m_buffer.getSize(); ++i)
			m_buffer[i].Initialize(width, height);

		m_writePosition = 0;
		m_readPosition = 0;

		return result;
	}

	bool VideoBuffer::Destroy()
	{
		Assert(m_initialized == true);

		bool result = true;

		m_initialized = false;

		for (size_t i = 0; i < m_buffer.getSize(); ++i)
			m_buffer[i].Destroy();

		m_buffer.setSize(0);

		return result;
	}

	VideoFrame* VideoBuffer::AllocateFrame()
	{
		Assert(m_buffer.getSize() > 0);

		++m_writePosition;

		if (m_writePosition >= m_buffer.getSize())
			m_writePosition = 0;

		if (m_writePosition == m_readPosition)
		{
			Debug::Print("Warning: Write position equals read position.");
			Assert(0);
		}

		return &m_buffer[m_writePosition];
	}

	VideoFrame* VideoBuffer::GetCurrentFrame()
	{
		Assert(m_buffer.getSize() > 0);

		return &m_buffer[m_readPosition];
	}

	void VideoBuffer::AdvanceToTime(double time)
	{
		Assert(m_buffer.getSize() > 0);

		// Skip as many frame necessary to reach the specified time.
		// Stop moving forward until the current write position (last written frame) is reached.

		int skipCount = 0;

		while (m_buffer[m_readPosition].m_time < time && m_readPosition != m_writePosition)
		{
			++m_readPosition;

			if (m_readPosition >= m_buffer.getSize())
				m_readPosition = 0;

			++skipCount;

			Debug::Print("Advancing frame.");
		}

		if (skipCount > 1)
		{
			Debug::Print("VIDEO: Warning: Skipped %d frames.", skipCount - 1);
		}
	}
};