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
