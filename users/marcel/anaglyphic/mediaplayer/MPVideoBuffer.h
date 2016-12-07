#pragma once

#include "MPMutex.h"
#include <ffmpeg/avcodec.h>
#include <list>

namespace MP
{
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
		void StoreFrame(VideoFrame * frame); ///< Add frame to consume list
		VideoFrame * GetCurrentFrame(); ///< Get current frame.
		void AdvanceToTime(double time); ///< Move to next frame.
		bool Depleted() const;
		bool IsFull() const;
		void Clear();

	private:
		Mutex m_mutex;

		std::list<VideoFrame*> m_freeList;
		std::list<VideoFrame*> m_consumeList;

		VideoFrame * m_currentFrame;

	public: // FIXME: Write accessor.
		bool m_initialized;
	};
};
