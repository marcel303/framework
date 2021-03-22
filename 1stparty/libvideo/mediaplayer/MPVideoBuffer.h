/*
	Copyright (C) 2020 Marcel Smit
	marcel303@gmail.com
	https://www.facebook.com/marcel.smit981

	Permission is hereby granted, free of charge, to any person
	obtaining a copy of this software and associated documentation
	files (the "Software"), to deal in the Software without
	restriction, including without limitation the rights to use,
	copy, modify, merge, publish, distribute, sublicense, and/or
	sell copies of the Software, and to permit persons to whom the
	Software is furnished to do so, subject to the following
	conditions:

	The above copyright notice and this permission notice shall be
	included in all copies or substantial portions of the Software.

	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
	EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
	OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
	NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
	HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
	WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
	FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
	OTHER DEALINGS IN THE SOFTWARE.
*/

#pragma once

#include "MPDebug.h"
#include "MPForward.h"
#include "MPMutex.h"
#include <list>
#include <stdint.h>

namespace MP
{
	class VideoFrame
	{
	public:
		VideoFrame();
		~VideoFrame();

		bool Initialize(
			const size_t width,
			const size_t height,
			const size_t pixelFormat);
		void Destroy();
		
		uint8_t * getY(int & sx, int & sy, int & pitch) const;
		uint8_t * getU(int & sx, int & sy, int & pitch) const;
		uint8_t * getV(int & sx, int & sy, int & pitch) const;
		uint8_t * getRGBA(int & sx, int & sy, int & pitch) const;
		
		size_t m_width = 0;
		size_t m_height = 0;
		size_t m_pixelFormat;

		AVFrame * m_frame           = nullptr;
		uint8_t * m_frameBuffer     = nullptr; // storage for m_frame contents
		int       m_frameBufferSize = 0;
		
		double m_time           = 0.0;
		bool   m_isFirstFrame   = false;
		bool   m_isValidForRead = false;

		bool m_initialized = false;
	};

	class VideoBuffer
	{
	public:
		~VideoBuffer();

		bool Initialize(
			const size_t width,
			const size_t height,
			const size_t pixelFormat);
		bool Destroy();
		bool IsInitialized() const;

		VideoFrame * AllocateFrame();
		void StoreFrame(VideoFrame * frame);
		VideoFrame * GetCurrentFrame();
		VideoFrame * PeekNextFrame();
		
		void AdvanceToTime(double time);
		bool Depleted() const;
		bool IsFull() const;
		void Clear();

	private:
		Mutex m_mutex;

		std::list<VideoFrame*> m_freeList;
		std::list<VideoFrame*> m_consumeList;

		VideoFrame * m_currentFrame = nullptr;

		bool m_initialized = false;
	};
};
