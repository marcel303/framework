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

#include "framework.h"

static SDL_atomic_t activeCount;
static SDL_atomic_t numRunning;
static SDL_atomic_t stopThreads;

static int threadFunction(void * obj)
{
	SDL_semaphore * sem = (SDL_semaphore*)obj;
	
	for (;;)
	{
		SDL_SemWait(sem);
		
		if (SDL_AtomicGet(&stopThreads) == 1)
			break;
		
		cpuTimingBlock(work);
		
		SDL_Delay(1);
		
		SDL_AtomicAdd(&activeCount, -1);
	}
	
	SDL_AtomicAdd(&activeCount, -1);
	SDL_AtomicAdd(&numRunning, -1);
	
	return 0;
}

void testThreading()
{
	const int kNumThreads = 6;
	
	SDL_semaphore * sem = SDL_CreateSemaphore(0);
	
	SDL_AtomicSet(&stopThreads, 0);
	
	SDL_AtomicSet(&numRunning, kNumThreads);
	
	SDL_Thread * thread[kNumThreads] = { };
	
	for (int i = 0; i < kNumThreads; ++i)
	{
		thread[i] = SDL_CreateThread(threadFunction, "WorkerThread", sem);
	}
	
	for (;;)
	{
		cpuTimingBlock(manage);
		
		SDL_AtomicAdd(&activeCount, kNumThreads);
		
		for (int i = 0; i < kNumThreads; ++i)
			SDL_SemPost(sem);
		
		framework.process();
		
		while (SDL_AtomicGet(&activeCount) != 0)
		{
			//SDL_Delay(1);
		}
		
		if (SDL_AtomicGet(&numRunning) == 0)
			break;
		
		if (keyboard.isDown(SDLK_SPACE))
			SDL_AtomicSet(&stopThreads, 1);
	}
	
	for (int i = 0; i < kNumThreads; ++i)
	{
		SDL_WaitThread(thread[i], nullptr);
	}
}
