/*
	Copyright (C) 2017 Marcel Smit
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

#if ENABLE_TURBOJPEG

#include <stdint.h>
#include <deque>

struct JpegLoadData;

struct VfxImageCpu;
struct VfxImageCpuData;

struct SDL_cond;
struct SDL_mutex;
struct SDL_Thread;

struct ImageCpuDelayLine
{
	struct JpegData
	{
		uint8_t * bytes;
		int numBytes;
		
		int sx;
		int sy;
		int numChannels;
		
		JpegData();
		~JpegData();
	};

	struct HistoryItem
	{
		VfxImageCpuData * imageData;
		JpegData * jpegData;
		double timestamp;

		HistoryItem();
		~HistoryItem();
	};

	struct WorkItem
	{
		VfxImageCpuData * imageData;
		int jpegQualityLevel;
		double timestamp;
		
		JpegData * jpegData;

		WorkItem();
		~WorkItem();
	};
	
	struct MemoryUsage
	{
		int numBytes;
		
		int numHistoryBytes;
		int numCachedImageBytes;
		int numSaveBufferBytes;
		
		int historySize;
	};
	
	uint8_t * saveBuffer;
	int saveBufferSize;
	
	std::deque<HistoryItem*> history;
	int maxHistorySize;
	int historySize;
	
	WorkItem work;
	
	bool stop;
	SDL_cond * workEvent;
	SDL_cond * doneEvent;
	SDL_mutex * mutex;
	SDL_Thread * thread;
	
	JpegLoadData * cachedLoadData;
	VfxImageCpu * cachedImage;

	ImageCpuDelayLine();
	~ImageCpuDelayLine();

	void init(const int maxHistorySize, const int saveBufferSize);
	void shut();
	
	void tick();
	
	int getLength() const;
	void setLength(const int length);
	
	void add(const VfxImageCpu & image, const int jpegQualityLevel, const double timestamp = 0.0, const bool useCompression = true);
	
	bool decode(const JpegData & jpegData, VfxImageCpuData & imageData, const bool glitch);
	bool get(const int offset, VfxImageCpuData & imageData, double * imageTimestamp = nullptr, const bool glitch = false);
	bool getByTimestamp(const double timestamp, VfxImageCpuData & imageData, double * imageTimestamp = nullptr, const bool glitch = false);
	
	void clearHistory();
	
	MemoryUsage getMemoryUsage() const;

	JpegData * compress(const VfxImageCpu & image, const int jpegQualityLevel);

	static int threadProc(void * arg);
	void threadMain();

	void compressWork(const VfxImageCpu & image, const int jpegQualityLevel, const double timestamp);
	void compressWait();
};

#endif
