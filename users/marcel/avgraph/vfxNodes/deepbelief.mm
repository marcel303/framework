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

#include "Debugging.h"
#include "deepbelief.h"
#include <DeepBelief/DeepBelief.h>
#include <SDL2/SDL.h>

#define DO_TIMING 1
#define DO_PRINTS 0

#if DO_TIMING
#include "Timer.h"
#endif

Deepbelief::Deepbelief()
	: isInitialized(false)
	, network(nullptr)
	, thread(nullptr)
	, mutex(nullptr)
	, workEvent(nullptr)
	, doneEvent(nullptr)
	, state()
{
}

Deepbelief::~Deepbelief()
{
	shut();
}

void Deepbelief::init(const char * networkFilename)
{
	shut();
	
	if (doInit(networkFilename) == false)
	{
		shut();
	}
}

bool Deepbelief::doInit(const char * networkFilename)
{
	network = jpcnn_create_network(networkFilename);
	Assert(network);
	
	if (network == nullptr)
		return false;
	
	Assert(mutex == nullptr);
	mutex = SDL_CreateMutex();

	Assert(workEvent == nullptr);
	workEvent = SDL_CreateCond();
	
	Assert(doneEvent == nullptr);
	doneEvent = SDL_CreateCond();
	
	if (mutex == nullptr || workEvent == nullptr || doneEvent == nullptr)
		return false;
	
	Assert(thread == nullptr);
	thread = SDL_CreateThread(threadMainProc, "DeepBelief", this);

	if (thread == nullptr)
		return false;
	
	Assert(isInitialized == false);
	isInitialized = true;
	
	return true;
}

void Deepbelief::shut()
{
	isInitialized = false;
	
	if (thread != nullptr)
	{
		SDL_Delay(1);
		
		SDL_LockMutex(mutex);
		{
			state.stop = true;

			int r = SDL_CondSignal(workEvent);
			Assert(r == 0);
		}
		SDL_UnlockMutex(mutex);

		SDL_WaitThread(thread, nullptr);
		thread = nullptr;
	}
	
	Assert(state.work == nullptr);
	state = State();
	
	if (workEvent)
	{
		SDL_DestroyCond(workEvent);
		workEvent = nullptr;
	}
	
	if (doneEvent)
	{
		SDL_DestroyCond(doneEvent);
		doneEvent = nullptr;
	}
	
	if (mutex)
	{
		SDL_DestroyMutex(mutex);
		mutex = nullptr;
	}
	
	if (network)
	{
		jpcnn_destroy_network(network);
		network = nullptr;
	}
}

void Deepbelief::process(const uint8_t * bytes, const int sx, const int sy, const int numChannels, const int pitch, const float certaintyTreshold)
{
	Work * oldWork = nullptr;
	
	Work * newWork = new Work();
	
	const int numBytes = sy * pitch;
	uint8_t * copiedBytes = new uint8_t[numBytes];
	memcpy(copiedBytes, bytes, numBytes);
	
	newWork->bytes = copiedBytes;
	newWork->sx = sx;
	newWork->sy = sy;
	newWork->numChannels = numChannels;
	newWork->pitch = pitch;
	newWork->certaintyTreshold = certaintyTreshold;
	
	SDL_LockMutex(mutex);
	{
		oldWork = state.work;
		state.work = newWork;

		int r = SDL_CondSignal(workEvent);
		Assert(r == 0);
	}
	SDL_UnlockMutex(mutex);

	if (oldWork)
	{
		delete oldWork;
		oldWork = nullptr;
	}
}

void Deepbelief::wait()
{
	SDL_LockMutex(mutex);
	{
		if (state.hasResult == false)
		{
			SDL_CondWait(doneEvent, mutex);
		}
		
		Assert(state.hasResult);
	}
	SDL_UnlockMutex(mutex);
}

bool Deepbelief::getResult(DeepbeliefResult & result)
{
	bool hasResult = false;

	SDL_LockMutex(mutex);
	{
		if (state.hasResult)
		{
			state.hasResult = false;
			
			result.predictions = std::move(state.result.predictions);
			
			result.bufferCreationTime = state.result.bufferCreationTime;
			result.classificationTime = state.result.classificationTime;
			result.sortTime = state.result.sortTime;
			
			state.result = DeepbeliefResult();

			hasResult = true;
		}
	}
	SDL_UnlockMutex(mutex);

	return hasResult;
}

int Deepbelief::threadMainProc(void * arg)
{
	Deepbelief * self = (Deepbelief*)arg;

	self->threadMain();

	return 0;
}

void Deepbelief::threadMain()
{
#if DO_PRINTS
	printf("thread: start\n");
#endif
	
	bool stop = false;
	
	SDL_LockMutex(mutex);
	
	do
	{
		Work * work = nullptr;
		
		SDL_CondWait(workEvent, mutex);
		{
			stop = state.stop;
			
			work = state.work;
			state.work = nullptr;
		}
		SDL_UnlockMutex(mutex);

		if (stop == false)
		{
		#if DO_PRINTS
			printf("thread: work\n");
		#endif
			
			std::vector<DeepbeliefPrediction> predictions;
			
			// process data using DeepBelief API
			
		#if DO_TIMING
			auto ti1 = g_TimerRT.TimeUS_get();
		#endif
			
			void * buffer = jpcnn_create_image_buffer_from_uint8_data(work->bytes, work->sx, work->sy, work->numChannels, work->pitch, false, false);
			Assert(buffer);
			
		#if DO_TIMING
			auto ti2 = g_TimerRT.TimeUS_get();
		#endif
			
		#if DO_TIMING
			auto tc1 = g_TimerRT.TimeUS_get();
		#endif
			
			if (buffer != nullptr)
			{
				float * predictionValues = nullptr;
				int predictionValuesLength = 0;
				char ** predictionsLabels = nullptr;
				int predictionsLabelsLength = 0;
				
				jpcnn_classify_image(network, buffer, JPCNN_RANDOM_SAMPLE, 0, &predictionValues, &predictionValuesLength, &predictionsLabels, &predictionsLabelsLength);
				
				for (int i = 0; i < predictionValuesLength; ++i)
				{
					const float predictionValue = predictionValues[i];

					if (predictionValue >= work->certaintyTreshold) // todo : make tweakable
					{
						DeepbeliefPrediction p;
						p.certainty = predictionValue;
						p.label = predictionsLabels[i % predictionsLabelsLength];
						
						predictions.push_back(p);
					}
				}
			}
			
		#if DO_TIMING
			auto tc2 = g_TimerRT.TimeUS_get();
		#endif
			
		#if DO_TIMING
			auto ts1 = g_TimerRT.TimeUS_get();
		#endif
			
			std::sort(predictions.begin(), predictions.end(), [](auto & p1, auto & p2) { return p1.certainty > p2.certainty; });
			
		#if DO_TIMING
			auto ts2 = g_TimerRT.TimeUS_get();
		#endif
			
			//
			
		#if DO_TIMING
			ti1 += g_TimerRT.TimeUS_get();
		#endif
			
			jpcnn_destroy_image_buffer(buffer);
			buffer = nullptr;
			
		#if DO_TIMING
			ti2 += g_TimerRT.TimeUS_get();
		#endif
			
			//
			
		#if DO_PRINTS
			printf("image creation took %.2fms\n", (ti2 - ti1) / 1000.0);
			printf("classification took %.2fms\n", (tc2 - tc1) / 1000.0);
			printf("sorting %d predictions took %.2fms\n", (int)predictions.size(), (ts2 - ts1) / 1000.0);
		#endif
			
			//
			
			delete work;
			work = nullptr;
		
			SDL_LockMutex(mutex);
			
			state.hasResult = true;
			state.result.predictions = std::move(predictions);
		#if DO_TIMING
			state.result.bufferCreationTime = (ti2 - ti1) / 1000.0;
			state.result.classificationTime = (tc2 - tc1) / 1000.0;
			state.result.sortTime = (ts2 - ts1) / 1000.0;
		#endif
			
			int r = SDL_CondSignal(doneEvent);
			Assert(r == 0);
		}
		else
		{
		#if DO_PRINTS
			printf("thread: stop\n");
		#endif
			
			delete work;
			work = nullptr;
			
			SDL_LockMutex(mutex);
			
			// we are done! let our thread end peacefully
		}
	} while (state.stop == false);
	
	SDL_UnlockMutex(mutex);
}
