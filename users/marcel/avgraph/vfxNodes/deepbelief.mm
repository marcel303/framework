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

#if ENABLE_DEEPBELIEF

#include "Debugging.h"
#include "deepbelief.h"
#include "Log.h"
#include <DeepBelief/DeepBelief.h>
#include <SDL2/SDL.h>

#define DO_TIMING 1
#define DO_PRINTS 0

#if DO_TIMING
#include "Timer.h"
#endif

Deepbelief::Deepbelief()
	: state(nullptr)
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
	Assert(state == nullptr);
	state = new State();
	
	state->networkFilename = networkFilename;
	
	state->mutex = SDL_CreateMutex();
	state->workEvent = SDL_CreateCond();
	state->doneEvent = SDL_CreateCond();
	
	if (state->mutex == nullptr || state->workEvent == nullptr || state->doneEvent == nullptr)
		return false;
	
	state->thread = SDL_CreateThread(threadMainProc, "DeepBelief", state);

	if (state->thread == nullptr)
		return false;
	
	SDL_DetachThread(state->thread);
	
	return true;
}

void Deepbelief::shut()
{
	if (state == nullptr)
		return;
	
	if (state->thread != nullptr)
	{
		SDL_LockMutex(state->mutex);
		{
			state->stop = true;

			int r = SDL_CondSignal(state->workEvent);
			Assert(r == 0);
		}
		SDL_UnlockMutex(state->mutex);
	}
	
	state = nullptr;
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
	
	SDL_LockMutex(state->mutex);
	{
		oldWork = state->work;
		state->work = newWork;

		int r = SDL_CondSignal(state->workEvent);
		Assert(r == 0);
	}
	SDL_UnlockMutex(state->mutex);

	if (oldWork)
	{
		delete oldWork;
		oldWork = nullptr;
	}
}

void Deepbelief::wait()
{
	SDL_LockMutex(state->mutex);
	{
		if (state->hasResult == false)
		{
			SDL_CondWait(state->doneEvent, state->mutex);
		}
		
		Assert(state->hasResult);
	}
	SDL_UnlockMutex(state->mutex);
}

bool Deepbelief::getResult(DeepbeliefResult & result)
{
	bool hasResult = false;

	SDL_LockMutex(state->mutex);
	{
		if (state->hasResult)
		{
			state->hasResult = false;
			
			result.predictions = std::move(state->result.predictions);
			
			result.bufferCreationTime = state->result.bufferCreationTime;
			result.classificationTime = state->result.classificationTime;
			result.sortTime = state->result.sortTime;
			
			state->result = DeepbeliefResult();

			hasResult = true;
		}
	}
	SDL_UnlockMutex(state->mutex);

	return hasResult;
}

int Deepbelief::threadMainProc(void * arg)
{
	State * state = (State*)arg;
	
	if (threadInit(state))
	{
		threadMain(state);
	}
	
	threadShut(state);

	delete state;
	state = nullptr;
	
	return 0;
}

bool Deepbelief::threadInit(State * state)
{
	LOG_DBG("creating deepbelief network", 0);
	
	if (state->networkFilename.empty())
	{
		return false;
	}
	else
	{
		state->network = jpcnn_create_network(state->networkFilename.c_str());
		Assert(state->network);
		
		if (state->network == nullptr)
			return false;
		
		state->isInitialized = true;
		
		return true;
	}
}

void Deepbelief::threadShut(State * state)
{
	Assert(state->work == nullptr);
	
	state->isInitialized = false;
	
	if (state->workEvent)
	{
		SDL_DestroyCond(state->workEvent);
		state->workEvent = nullptr;
	}
	
	if (state->doneEvent)
	{
		SDL_DestroyCond(state->doneEvent);
		state->doneEvent = nullptr;
	}
	
	if (state->mutex)
	{
		SDL_DestroyMutex(state->mutex);
		state->mutex = nullptr;
	}
	
	if (state->network)
	{
		LOG_DBG("destroying deepbelief network", 0);
		
		jpcnn_destroy_network(state->network);
		state->network = nullptr;
	}
}

void Deepbelief::threadMain(State * state)
{
#if DO_PRINTS
	printf("thread: start\n");
#endif
	
	SDL_LockMutex(state->mutex);
	
	for (;;)
	{
		Work * work = state->work;
		state->work = nullptr;
		
		if (state->stop)
		{
			delete work;
			work = nullptr;
			
			break;
		}
		
		if (work != nullptr)
		{
			SDL_UnlockMutex(state->mutex);
			
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
				
				jpcnn_classify_image(
					state->network, buffer, JPCNN_RANDOM_SAMPLE, 0,
					&predictionValues, &predictionValuesLength, &predictionsLabels, &predictionsLabelsLength);
				
				for (int i = 0; i < predictionValuesLength; ++i)
				{
					const float predictionValue = predictionValues[i];

					if (predictionValue >= work->certaintyTreshold)
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
		
			SDL_LockMutex(state->mutex);
		
			state->hasResult = true;
			state->result.predictions = std::move(predictions);
		#if DO_TIMING
			state->result.bufferCreationTime = (ti2 - ti1) / 1000.0;
			state->result.classificationTime = (tc2 - tc1) / 1000.0;
			state->result.sortTime = (ts2 - ts1) / 1000.0;
		#endif
			
			int r = SDL_CondSignal(state->doneEvent);
			Assert(r == 0);
		}
		
		if (state->stop == false && state->work == nullptr) // 'if no more work to be done and should go idle'
		{
			SDL_CondWait(state->workEvent, state->mutex);
		}
	}
	
	SDL_UnlockMutex(state->mutex);
}

#endif
