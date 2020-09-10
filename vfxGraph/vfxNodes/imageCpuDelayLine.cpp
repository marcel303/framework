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

#if ENABLE_TURBOJPEG

#include "imageCpuDelayLine.h"
#include "jpegLoader.h"
#include "ofxJpegGlitch/ofxJpegGlitch.h"
#include "vfxNodeBase.h"
#include <SDL2/SDL.h>

static void copyImage(const VfxImageCpu & image, VfxImageCpuData & imageData)
{
	imageData.allocOnSizeChange(image.sx, image.sy, image.numChannels);
	
	for (int i = 0; i < image.numChannels; ++i)
	{
		const VfxImageCpu::Channel & srcChannel = image.channel[i];
		VfxImageCpu::Channel & dstChannel = imageData.image.channel[i];
		
		for (int y = 0; y < image.sy; ++y)
		{
			const uint8_t * __restrict srcBytes = srcChannel.data + y * srcChannel.pitch;
			uint8_t * __restrict dstBytes = (uint8_t*)dstChannel.data + y * dstChannel.pitch;
			
			memcpy(dstBytes, srcBytes, image.sx);
		}
	}
}

//

ImageCpuDelayLine::JpegData::JpegData()
	: bytes(nullptr)
	, numBytes(0)
	, sx(0)
	, sy(0)
	, numChannels(0)
{
}

ImageCpuDelayLine::JpegData::~JpegData()
{
	delete[] bytes;
	bytes = nullptr;
	numBytes = 0;
	
	sx = 0;
	sy = 0;
	numChannels = 0;
}

//

ImageCpuDelayLine::HistoryItem::HistoryItem()
	: imageData(nullptr)
	, jpegData(nullptr)
	, timestamp(0.0)
{
}

ImageCpuDelayLine::HistoryItem::~HistoryItem()
{
	delete imageData;
	imageData = nullptr;

	delete jpegData;
	jpegData = nullptr;
	
	timestamp = 0.0;
}

//

ImageCpuDelayLine::WorkItem::WorkItem()
	: imageData(nullptr)
	, imageDataIsConsumed(true)
	, jpegQualityLevel(0)
	, timestamp(0.0)
	, jpegData(nullptr)
	, isDone(false)
{
}

ImageCpuDelayLine::WorkItem::~WorkItem()
{
	isDone = false;
	
	delete jpegData;
	jpegData = nullptr;
	
	jpegQualityLevel = 0;
	
	imageDataIsConsumed = true;
	
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
	
	Assert(work.imageData == nullptr);
	Assert(work.imageDataIsConsumed);
	Assert(work.jpegQualityLevel == 0);
	Assert(work.jpegData == nullptr);
	Assert(work.isDone == false);
	work = WorkItem();
	
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
	
	Assert(workEvent != nullptr);
	Assert(doneEvent != nullptr);
	Assert(mutex != nullptr);
	Assert(thread != nullptr);
}

void ImageCpuDelayLine::shut()
{
	if (mutex != nullptr)
	{
		compressWait();
	}
	
	delete cachedLoadData;
	cachedLoadData = nullptr;
	
	if (thread != nullptr)
	{
		Verify(SDL_LockMutex(mutex) == 0);
		{
			stop = true;
			
			Verify(SDL_CondSignal(workEvent) == 0);
		}
		Verify(SDL_UnlockMutex(mutex) == 0);

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
	
	Assert(work.imageData == nullptr);
	Assert(work.imageDataIsConsumed);
	Assert(work.jpegQualityLevel == 0);
	Assert(work.jpegData == nullptr);
	Assert(work.isDone == false);
	work = WorkItem();

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

void ImageCpuDelayLine::tick()
{
	compressWait();
}

int ImageCpuDelayLine::getLength() const
{
	return maxHistorySize;
}

void ImageCpuDelayLine::setLength(const int length)
{
	compressWait();
	
	//
	
	maxHistorySize = length;
	
	while (historySize > maxHistorySize)
	{
		HistoryItem * item = history.back();
		delete item;
		item = nullptr;

		history.pop_back();
		
		historySize--;
	}
}

void ImageCpuDelayLine::add(const VfxImageCpu & image, const int jpegQualityLevel, const double timestamp, const bool useCompression)
{
	// note : compressWait may add to the history. we need to wait here before we add another item
	
	compressWait();
	
	// note : turbojpeg doesn't like it when we try to compress a 0x0 sized image. just use the non-compressed
	//        code path to avoid issues
	
	if (useCompression && (image.sx > 0 && image.sy > 0))
	{
		compressWork(image, jpegQualityLevel, timestamp);
	}
	else
	{
		HistoryItem * historyItem = nullptr;
		
		if (historySize == maxHistorySize)
		{
			historyItem = history.back();

			history.pop_back();
		}
		else
		{
			historySize++;
			
			historyItem = new HistoryItem();
		}
		
		delete historyItem->jpegData;
		historyItem->jpegData = nullptr;
		
		if (historyItem->imageData == nullptr)
			historyItem->imageData = new VfxImageCpuData();
		
		copyImage(image, *historyItem->imageData);
		historyItem->timestamp = timestamp;
		
		history.push_front(historyItem);
	}
}

bool ImageCpuDelayLine::decode(const JpegData & jpegData, VfxImageCpuData & imageData, const bool glitch, const float glitchiness)
{
	if (jpegData.bytes == nullptr)
	{
		return false;
	}
	else
	{
		// decode JPEG data
		
		if (glitch && glitchiness > 0.f)
		{
			ofxJpegGlitch glitch;
			ofBuffer buffer(jpegData.bytes, jpegData.numBytes);
			glitch.setJpegBuffer(buffer);
			glitch.setDataGlitchness(0);
			glitch.setDHTGlitchness(0);
			//glitch.setQNGlitchness(0);
			glitch.setQNGlitchness(glitchiness * ofxJpegGlitch::kMaxGlitchiness);
			glitch.glitch();
		}
		
	#if 0
		imageData.allocOnSizeChange(jpegData.sx, jpegData.sy, jpegData.numChannels, true);
		const int numImageDataBytes = imageData.image.sy * imageData.image.channel[0].pitch;
		
		if (loadImage_turbojpeg(jpegData.bytes, jpegData.numBytes, imageData.data, numImageDataBytes, jpegData.numChannels == 1) == false)
		{
			return false;
		}
		else
		{
			return true;
		}
	#else
		if (loadImage_turbojpeg(jpegData.bytes, jpegData.numBytes, *cachedLoadData, jpegData.numChannels == 1) == false)
		{
			return false;
		}
		else
		{
			if (jpegData.numChannels == 1)
			{
				imageData.allocOnSizeChange(cachedLoadData->sx, cachedLoadData->sy, 1);
				
				VfxImageCpu::deinterleave1(
					cachedLoadData->buffer,
					cachedLoadData->sx,
					cachedLoadData->sy,
					1,
					cachedLoadData->sx * 1,
					imageData.image.channel[0]);
			}
			else
			{
				// deinterleave data
				
				imageData.allocOnSizeChange(cachedLoadData->sx, cachedLoadData->sy, 4);
				
				VfxImageCpu::deinterleave4(
					cachedLoadData->buffer,
					cachedLoadData->sx,
					cachedLoadData->sy,
					1,
					cachedLoadData->sx * 4,
					imageData.image.channel[0],
					imageData.image.channel[1],
					imageData.image.channel[2],
					imageData.image.channel[3]);
			}
			
			return true;
		}
	#endif
	}
}

bool ImageCpuDelayLine::get(const int offset, VfxImageCpuData & imageData, double * imageTimestamp, const bool glitch, const float glitchiness)
{
	JpegData jpegData;
	bool gotImageData = false;
	
	// make a copy of the data, so we can leave the mutex quickly again and let the worker thread do it's encode thing
	
	if (offset >= 0 && offset < historySize)
	{
		HistoryItem * item = history[offset];
		
		if (item->jpegData != nullptr)
		{
			memcpy(&jpegData, item->jpegData, sizeof(jpegData));
			jpegData.bytes = new uint8_t[jpegData.numBytes];
			memcpy(jpegData.bytes, item->jpegData->bytes, item->jpegData->numBytes);
			
			if (imageTimestamp != nullptr)
			{
				*imageTimestamp = item->timestamp;
			}
		}
		else
		{
			gotImageData = true;
			
			copyImage(item->imageData->image, imageData);
		}
	}
	
	if (gotImageData)
		return true;
	else
		return decode(jpegData, imageData, glitch, glitchiness);
}

bool ImageCpuDelayLine::getByTimestamp(const double timestamp, VfxImageCpuData & imageData, double * imageTimestamp, const bool glitch, const float glitchiness)
{
	JpegData jpegData;
	
	// make a copy of the data, so we can leave the mutex quickly again and let the worker thread do it's encode thing
	
	HistoryItem * item = nullptr;
	
	for (auto & h : history)
	{
		if (h->timestamp < timestamp)
			break;
		
		item = h;
	}
	
	if (item != nullptr)
	{
		memcpy(&jpegData, item->jpegData, sizeof(jpegData));
		jpegData.bytes = new uint8_t[item->jpegData->numBytes];
		memcpy(jpegData.bytes, item->jpegData->bytes, item->jpegData->numBytes);
		
		if (imageTimestamp != nullptr)
		{
			*imageTimestamp = item->timestamp;
		}
	}
	
	return decode(jpegData, imageData, glitch, glitchiness);
}

void ImageCpuDelayLine::clearHistory()
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

ImageCpuDelayLine::MemoryUsage ImageCpuDelayLine::getMemoryUsage() const
{
	MemoryUsage result;
	
	{
		result.numHistoryBytes = 0;
		
		for (auto & h : history)
		{
			if (h->jpegData != nullptr)
				result.numHistoryBytes += h->jpegData->numBytes;
			if (h->imageData != nullptr)
				result.numHistoryBytes += h->imageData->image.getMemoryUsage();
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
	}
	
	if (mutex != nullptr)
	{
		Verify(SDL_LockMutex(mutex) == 0);
		{
			if (work.imageDataIsConsumed == false)
			{
				result.numSaveBufferBytes += work.imageData->image.getMemoryUsage();
			}
			
			if (work.jpegData != nullptr)
			{
				result.numSaveBufferBytes += work.jpegData->numBytes;
			}
			
			//
			
			result.historySize = historySize;
		}
		Verify(SDL_UnlockMutex(mutex) == 0);
	}
	
	result.numBytes =
		result.numHistoryBytes +
		result.numCachedImageBytes +
		result.numSaveBufferBytes;
	
	return result;
}

ImageCpuDelayLine::JpegData * ImageCpuDelayLine::compress(const VfxImageCpu & image, const int jpegQualityLevel)
{
	const void * srcBuffer = nullptr;
	int srcBufferSize = 0;
	
	std::vector<uint8_t> temp;
	
	if (image.numChannels == 1)
	{
		// note : avoid temp alloc and copy if pitch allows it
		
		if (image.channel[0].pitch == image.sx)
		{
			srcBuffer = image.channel[0].data;
			srcBufferSize = image.sx * image.sy * 1;
		}
		else
		{
			// todo : specify pitch when calling saveImage_turbojpeg. turbojpeg supports it. remove temp alloc
			
			temp.resize(image.sx * image.sy * 1);
			
			VfxImageCpu::interleave1(
				image.channel[0],
				&temp[0], image.sx * 1,
				image.sx, image.sy);
			
			srcBuffer = (image.sx > 0 && image.sy > 0) ? &temp[0] : nullptr;
			srcBufferSize = temp.size();
		}
	}
	else
	{
		temp.resize(image.sx * image.sy * 4);
		
		VfxImageCpu::interleave4(
			image.channel[0],
			image.channel[1],
			image.channel[2],
			image.channel[3],
			&temp[0], image.sx * 4,
			image.sx, image.sy);
		
		srcBuffer = (image.sx > 0 && image.sy > 0) ? &temp[0] : nullptr;
		srcBufferSize = temp.size();
	}
	
	const int srcSx = image.sx;
	const int srcSy = image.sy;
	const bool srcIsColor = image.numChannels >= 3;
	
	void * dstBuffer = saveBuffer;
	int dstBufferSize = saveBufferSize;
	
	if (saveImage_turbojpeg(srcBuffer, srcBufferSize, srcSx, srcSy, srcIsColor, jpegQualityLevel, dstBuffer, dstBufferSize))
	{
		JpegData * jpegData = new JpegData();
		jpegData->bytes = new uint8_t[dstBufferSize];
		jpegData->numBytes = dstBufferSize;
		jpegData->sx = image.sx;
		jpegData->sy = image.sy;
		jpegData->numChannels = image.numChannels;
		
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
	Verify(SDL_LockMutex(mutex) == 0);

	for (;;)
	{
		Verify(SDL_CondWait(workEvent, mutex) == 0);
		
		// note : we check explicitly for all of the conditions here, because they may not all be satistfied!
		//        there is a peculiarity when using the debugger on macOS, where when the app is paused and
		//        then resumed, the SDL_CondSignal call succeeds, without the main thread explicitly setting the event
		
		if (stop)
		{
			delete work.imageData;
			
			Verify(SDL_CondSignal(doneEvent) == 0);
			
			break;
		}
		
		if (work.imageData != nullptr)
		{
			VfxImageCpuData * imageData = work.imageData;
			const int jpegQualityLevel = work.jpegQualityLevel;
		
			JpegData * jpegData = nullptr;
			
			work.imageDataIsConsumed = true;
			
			Verify(SDL_UnlockMutex(mutex) == 0);
			{
				if (stop == false)
				{
					jpegData = compress(imageData->image, jpegQualityLevel);
				}

				delete imageData;
				//imageData = nullptr; // not set to nullptr, so we can validate the pointer later, to check work.imageData was left untouched
			}
			Verify(SDL_LockMutex(mutex) == 0);
			
			// store the result
			if (jpegData != nullptr)
			{
				Assert(work.jpegData == nullptr);
				work.jpegData = jpegData;
			}
			
			// check if these work members were left untouched
			Assert(work.imageData == imageData);
			Assert(work.imageDataIsConsumed);
			Assert(work.jpegQualityLevel == jpegQualityLevel);
			Assert(work.isDone == false);
			
			// tell the main thread we're done
			work.isDone = true;
			
			Verify(SDL_CondSignal(doneEvent) == 0);
		}
	}

	Verify(SDL_UnlockMutex(mutex) == 0);
}

void ImageCpuDelayLine::compressWork(const VfxImageCpu & image, const int jpegQualityLevel, const double timestamp)
{
	// make a copy of the image
	
	VfxImageCpuData * imageData = new VfxImageCpuData();
	copyImage(image, *imageData);
	
	// kick the compression thread
	
	Verify(SDL_LockMutex(mutex) == 0);
	{
		Assert(work.imageData == nullptr);
		Assert(work.imageDataIsConsumed);
		Assert(work.jpegQualityLevel == 0);
		Assert(work.timestamp == 0.0);
		Assert(work.isDone == false);
		
		work.imageData = imageData;
		work.imageDataIsConsumed = false;
		work.jpegQualityLevel = jpegQualityLevel;
		work.timestamp = timestamp;
		work.isDone = false;
		
		Verify(SDL_CondSignal(workEvent) == 0);
	}
	Verify(SDL_UnlockMutex(mutex) == 0);
}

void ImageCpuDelayLine::compressWait()
{
	JpegData * jpegData = nullptr;
	double timestamp = 0.0;
	
	Verify(SDL_LockMutex(mutex) == 0);
	{
		if (work.imageData != nullptr)
		{
			Verify(SDL_CondWait(doneEvent, mutex) == 0);
			
			// is the compression work done?
			
			if (work.isDone)
			{
				// if so, finish up the work
				
				// steal the data
				jpegData = work.jpegData;
				timestamp = work.timestamp;
				
				// and clear the work item
				work.imageData = nullptr;
				work.jpegQualityLevel = 0;
				work.jpegData = nullptr;
				work.isDone = false;
				work.timestamp = 0.0;
				
				Assert(work.imageData == nullptr);
				Assert(work.imageDataIsConsumed);
				Assert(work.jpegQualityLevel == 0);
				Assert(work.jpegData == 0);
				Assert(work.isDone == false);
				Assert(work.timestamp == 0.0);
			}
		}
	}
	Verify(SDL_UnlockMutex(mutex) == 0);
	
	if (jpegData != nullptr)
	{
		// add the compressed data to the history
		
		HistoryItem * historyItem = new HistoryItem();
		historyItem->jpegData = jpegData;
		historyItem->timestamp = timestamp;
		
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

#endif
