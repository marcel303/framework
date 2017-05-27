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

		bool Initialize(const size_t width, const size_t height, const size_t pixelFormat);
		void Destroy();
		
		uint8_t * getY(int & sx, int & sy, int & pitch) const;
		uint8_t * getU(int & sx, int & sy, int & pitch) const;
		uint8_t * getV(int & sx, int & sy, int & pitch) const;
		
		size_t m_width;
		size_t m_height;
		size_t m_pixelFormat;

		AVFrame * m_frame;
		uint8_t * m_frameBuffer;
		int m_frameBufferSize; // todo : remove once decode buffer optimize issue is fixed ? or only compile in debug
		double m_time;
		bool m_isFirstFrame;
		bool m_isValidForRead; // todo : remove once decode buffer optimize issue is fixed ?

		bool m_initialized;
	};

	class VideoBuffer
	{
	public:
		VideoBuffer();
		~VideoBuffer();

		bool Initialize(const size_t width, const size_t height, const size_t pixelFormat);
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
