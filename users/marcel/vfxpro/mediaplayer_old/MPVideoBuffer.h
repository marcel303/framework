#pragma once

#include "types.h"
#include <ffmpeg_old/avcodec.h>
#include <list>

namespace MP
{
	// TODO: Move to cpp/h.
	class VideoFrame
	{
	public:
		VideoFrame();
		~VideoFrame();

		void Initialize(const size_t width, const size_t height);
		void Destroy();

		size_t m_width;
		size_t m_height;

		AVFrame * m_frame;
		uint8_t * m_frameBuffer;
		double m_time; // Time at which the frame should be presented.

		bool m_initialized;
	};

	class VideoBuffer
	{
	public:
		VideoBuffer();
		~VideoBuffer();

		bool Initialize(const size_t width, const size_t height);
		bool Destroy();

		VideoFrame * AllocateFrame(); ///< Allocate new frame.
		VideoFrame * GetCurrentFrame(); ///< Get current frame.
		void AdvanceToTime(double time); ///< Move to next frame.
		bool Depleted() const;
		bool IsFull() const;
		void Clear();

	private:
		std::list<VideoFrame*> m_freeList;
		std::list<VideoFrame*> m_consumeList;

		VideoFrame * m_currentFrame;

	public: // FIXME: Write accessor.
		bool m_initialized;
	};
};
