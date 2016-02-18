#pragma once

#include "types.h"
#include <libavcodec/avcodec.h>

namespace MP
{
	// TODO: Move to cpp/h.
	class VideoFrame
	{
	public:
		VideoFrame();
		~VideoFrame();

		void Initialize(size_t width, size_t height);
		void Destroy();

		size_t m_width;
		size_t m_height;

		AVFrame* m_frame;
		uint8_t* m_frameBuffer;
		double m_time; // Time at which the frame should be presented.

		bool m_initialized;
	};

	class VideoBuffer
	{
	public:
		VideoBuffer();

		bool Initialize(size_t width, size_t height);
		bool Destroy();

		VideoFrame* AllocateFrame(); ///< Allocate new frame.
		VideoFrame* GetCurrentFrame(); ///< Get current frame.
		void AdvanceToTime(double time); ///< Move to next frame.

	private:
		Array<VideoFrame> m_buffer; ///< Queued video frames. TODO: Create video buffer class with reusable video frames.

		size_t m_writePosition;
		size_t m_readPosition;

	public: // FIXME: Write accessor.
		bool m_initialized;
	};
};
