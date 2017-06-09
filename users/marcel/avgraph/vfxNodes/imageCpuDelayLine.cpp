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

#include "imageCpuDelayLine.h"
#include "jpegLoader.h"
#include "vfxNodeBase.h"
#include <SDL2/SDL.h>

ImageCpuDelayLine::JpegData::JpegData()
	: bytes(nullptr)
	, numBytes(0)
{
}

ImageCpuDelayLine::JpegData::~JpegData()
{
	delete[] bytes;
	bytes = nullptr;
	numBytes = 0;
}

//

ImageCpuDelayLine::HistoryItem::HistoryItem()
	: image(nullptr)
	, jpegData(nullptr)
{
}

ImageCpuDelayLine::HistoryItem::~HistoryItem()
{
	delete image;
	image = nullptr;

	delete jpegData;
	jpegData = nullptr;
}

//

ImageCpuDelayLine::WorkItem::WorkItem()
	: imageData(nullptr)
{
}

ImageCpuDelayLine::WorkItem::~WorkItem()
{
	delete imageData;
	imageData = nullptr;
}

//

ImageCpuDelayLine::ImageCpuDelayLine()
	: saveBuffer(nullptr)
	, saveBufferSize(0)
	, history()
	, maxHistorySize(0)
	, historySize(0)
	, work()
	, stop(false)
	, workEvent(nullptr)
	, doneEvent(nullptr)
	, mutex(nullptr)
	, thread(nullptr)
	, cachedLoadData(nullptr)
	, cachedImage(nullptr)
{
}

ImageCpuDelayLine::~ImageCpuDelayLine()
{
	shut();
}

void ImageCpuDelayLine::init(const int _maxHistorySize, const int _saveBufferSize)
{
	shut();

	//
	
	Assert(saveBuffer == nullptr);
	saveBuffer = new uint8_t[_saveBufferSize];
	saveBufferSize = _saveBufferSize;
	
	Assert(history.empty());
	
	Assert(maxHistorySize == 0);
	maxHistorySize = _maxHistorySize;

	Assert(historySize == 0);
	historySize = 0;
	
	Assert(work == nullptr);
	work = nullptr;
	
	Assert(stop == false);
	stop = false;

	Assert(workEvent == nullptr);
	workEvent = SDL_CreateCond();

	Assert(doneEvent == nullptr);
	doneEvent = SDL_CreateCond();

	Assert(mutex == nullptr);
	mutex = SDL_CreateMutex();

	Assert(thread == nullptr);
	thread = SDL_CreateThread(threadProc, "ImageCpuDelayCompress", this);
	
	Assert(cachedLoadData == nullptr);
	cachedLoadData = new JpegLoadData();
	
	Assert(cachedImage == nullptr);
	cachedImage = new VfxImageCpu();
}

void ImageCpuDelayLine::shut()
{
	delete cachedLoadData;
	cachedLoadData = nullptr;
	
	delete cachedImage;
	cachedImage = nullptr;
	
	if (thread != nullptr)
	{
		SDL_LockMutex(mutex);
		{
			stop = true;
		}
		SDL_UnlockMutex(mutex);
		
		SDL_CondSignal(workEvent);

		SDL_WaitThread(thread, nullptr);
		thread = nullptr;
		
		stop = false;
	}
	
	if (mutex != nullptr)
	{
		SDL_DestroyMutex(mutex);
		mutex = nullptr;
	}

	if (doneEvent != nullptr)
	{
		SDL_DestroyCond(doneEvent);
		doneEvent = nullptr;
	}

	if (workEvent != nullptr)
	{
		SDL_DestroyCond(workEvent);
		workEvent = nullptr;
	}
	
	Assert(work == nullptr);
	delete work;
	work = nullptr;

	while (!history.empty())
	{
		HistoryItem * item = history.back();
		delete item;
		item = nullptr;

		history.pop_back();
	}

	historySize = 0;
	maxHistorySize = 0;
	
	delete[] saveBuffer;
	saveBuffer = nullptr;
	saveBufferSize = 0;
}

void ImageCpuDelayLine::add(const VfxImageCpu & image, const int jpegQualityLevel)
{
	compressWait();
	
	compressWork(image, jpegQualityLevel);
}

VfxImageCpu * ImageCpuDelayLine::get(const int offset)
{
	JpegData jpegData;
	
	// make a copy of the data, so we can leave the mutex quickly again and let the worker thread do it's encode thing
	
	SDL_LockMutex(mutex);
	{
		if (offset >= 0 && offset < historySize)
		{
			HistoryItem * item = history[offset];
			
			jpegData.bytes = new uint8_t[item->jpegData->numBytes];
			jpegData.numBytes = item->jpegData->numBytes;
			
			memcpy(jpegData.bytes, item->jpegData->bytes, item->jpegData->numBytes);
		}
	}
	SDL_UnlockMutex(mutex);
	
	if (jpegData.bytes == nullptr)
	{
		return nullptr;
	}
	else
	{
		// decode JPEG data
		
		if (loadImage_turbojpeg(jpegData.bytes, jpegData.numBytes, *cachedLoadData) == false)
		{
			return nullptr;
		}
		else
		{
			cachedImage->setDataRGBA8(cachedLoadData->buffer, cachedLoadData->sx, cachedLoadData->sy, 4, cachedLoadData->sx * 4);
			
			return cachedImage;
		}
	}
}

void ImageCpuDelayLine::clearHistory()
{
	SDL_LockMutex(mutex);
	{
		while (!history.empty())
		{
			HistoryItem * item = history.back();
			delete item;
			item = nullptr;

			history.pop_back();
		}
		
		historySize = 0;
	}
	SDL_UnlockMutex(mutex);
}

ImageCpuDelayLine::MemoryUsage ImageCpuDelayLine::getMemoryUsage() const
{
	MemoryUsage result;
	
	SDL_LockMutex(mutex);
	{
		result.numHistoryBytes = 0;
		
		for (auto & h : history)
		{
			result.numHistoryBytes += h->jpegData->numBytes;
		}
		
		//
		
		result.numCachedImageBytes = 0;
		
		if (cachedLoadData != nullptr)
		{
			result.numCachedImageBytes += cachedLoadData->bufferSize;
		}
		
		//
		
		result.numSaveBufferBytes = 0;
		
		result.numSaveBufferBytes += saveBufferSize;
		
		if (work != nullptr)
		{
			result.numSaveBufferBytes += work->imageData->image.getMemoryUsage();
		}
		
		//
		
		result.historySize = historySize;
	}
	SDL_UnlockMutex(mutex);
	
	result.numBytes = result.numHistoryBytes + result.numCachedImageBytes + result.numSaveBufferBytes;
	
	return result;
}

ImageCpuDelayLine::JpegData * ImageCpuDelayLine::compress(const VfxImageCpu & image, const int jpegQualityLevel)
{
	const void * srcBuffer = image.channel[0].data;
	const int srcBufferSize = image.sx * image.sy * image.numChannels;
	const int srcSx = image.sx;
	const int srcSy = image.sy;
	const bool srcIsColor = image.numChannels == 4;
	
	void * dstBuffer = saveBuffer;
	int dstBufferSize = saveBufferSize;
	
	if (saveImage_turbojpeg(srcBuffer, srcBufferSize, srcSx, srcSy, srcIsColor, jpegQualityLevel, dstBuffer, dstBufferSize))
	{
		JpegData * jpegData = new JpegData();
		jpegData->bytes = new uint8_t[dstBufferSize];
		jpegData->numBytes = dstBufferSize;
		
		memcpy(jpegData->bytes, dstBuffer, dstBufferSize);
		
		return jpegData;
	}
	else
	{
		return nullptr;
	}
}

int ImageCpuDelayLine::threadProc(void * arg)
{
	ImageCpuDelayLine * self = (ImageCpuDelayLine*)arg;

	self->threadMain();
	
	return 0;
}

void ImageCpuDelayLine::threadMain()
{
	SDL_LockMutex(mutex);

	for (;;)
	{
		WorkItem * w = work;
		work = nullptr;
		
		if (stop)
		{
			delete w;
			w = nullptr;
			
			break;
		}
		
		if (w != nullptr)
		{
			HistoryItem * historyItem = nullptr;

			SDL_UnlockMutex(mutex);
			{
				if (stop == false)
				{
					Assert(w != nullptr);
					Assert(w->imageData != nullptr);

					JpegData * jpegData = compress(w->imageData->image, w->jpegQualityLevel);
					
					if (jpegData != nullptr)
					{
						historyItem = new HistoryItem();
						historyItem->jpegData = jpegData;
					}
				}

				delete w;
				w = nullptr;
			}
			SDL_LockMutex(mutex);

			if (historyItem != nullptr)
			{
				if (historySize == maxHistorySize)
				{
					HistoryItem * item = history.back();
					delete item;
					item = nullptr;

					history.pop_back();
				}
				else
				{
					historySize++;
				}

				history.push_front(historyItem);
			}
		}
		
		SDL_CondSignal(doneEvent);
		
		if (stop == false && work == nullptr) // 'if no more work to be done and should go idle'
		{
			SDL_CondWait(workEvent, mutex);
		}
	}

	SDL_UnlockMutex(mutex);
}

void ImageCpuDelayLine::compressWork(const VfxImageCpu & image, const int jpegQualityLevel)
{
	if (image.isInterleaved == false)
		return;
	
	// todo : make a copy of the image
	
	VfxImageCpuData * imageData = new VfxImageCpuData();
	imageData->alloc(image.sx, image.sy, image.numChannels, true);
	
	const VfxImageCpu::Channel & srcChannel = image.channel[0];
	VfxImageCpu::Channel & dstChannel = imageData->image.channel[0];
	
	for (int y = 0; y < image.sy; ++y)
	{
		const uint8_t * __restrict srcBytes = srcChannel.data + y * srcChannel.pitch;
		uint8_t * __restrict dstBytes = (uint8_t*)dstChannel.data + y * dstChannel.pitch;
		
		memcpy(dstBytes, srcBytes, image.sx * image.numChannels);
	}
	
	SDL_LockMutex(mutex);
	{
		WorkItem * w = new WorkItem();
		w->imageData = imageData;
		w->jpegQualityLevel = jpegQualityLevel;
		
		Assert(work == nullptr);
		work = w;

		SDL_CondSignal(workEvent);
	}
	SDL_UnlockMutex(mutex);
}

void ImageCpuDelayLine::compressWait()
{
	SDL_LockMutex(mutex);
	
	if (work != nullptr)
	{
		SDL_CondWait(doneEvent, mutex);
	}
	
	SDL_UnlockMutex(mutex);
}
