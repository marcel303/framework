#pragma once

#include "MPForward.h"
#include "MPMutex.h"
#include <list>

#include <SDL2/SDL.h> // fixme : abstract away

namespace MP
{
	class VideoFrame
	{
	public:
		VideoFrame();
		~VideoFrame();

		bool Initialize(const size_t width, const size_t height);
		void Destroy();
		
		uint8_t * getY(int & sx, int & sy, int & pitch) const;
		uint8_t * getU(int & sx, int & sy, int & pitch) const;
		uint8_t * getV(int & sx, int & sy, int & pitch) const;
		
		size_t m_width;
		size_t m_height;

		AVFrame * m_frame;
		uint8_t * m_frameBuffer;
		double m_time;
		bool m_isFirstFrame;

		bool m_initialized;
	};

	class VideoBuffer
	{
	public:
		VideoBuffer();
		~VideoBuffer();

		bool Initialize(const size_t width, const size_t height);
		bool Destroy();
		bool IsInitialized() const;

		VideoFrame * AllocateFrame();
		void StoreFrame(VideoFrame * frame);
		VideoFrame * GetCurrentFrame();
		void AdvanceToTime(double time);
		bool Depleted() const;
		bool IsFull() const;
		void Clear();

	private:
		Mutex m_mutex;

		std::list<VideoFrame*> m_freeList;
		std::list<VideoFrame*> m_consumeList;

		VideoFrame * m_currentFrame;

		bool m_initialized;
	};
};
