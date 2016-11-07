#include "MPMutex.h"

namespace MP
{
	// Mutex
	Mutex::Mutex()
		: mutex(nullptr)
	{
		mutex = SDL_CreateMutex();
	}

	Mutex::~Mutex()
	{
		SDL_DestroyMutex(mutex);
		mutex = nullptr;
	}

	void Mutex::Lock()
	{
		SDL_LockMutex(mutex);
	}

	void Mutex::Unlock()
	{
		SDL_UnlockMutex(mutex);
	}
}
