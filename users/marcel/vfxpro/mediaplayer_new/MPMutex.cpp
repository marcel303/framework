#include "MPDebug.h"
#include "MPMutex.h"
#include <SDL2/SDL.h>

namespace MP
{
	Mutex::Mutex()
		: mutex(nullptr)
	{
		mutex = SDL_CreateMutex();
	}

	Mutex::~Mutex()
	{
		if (mutex != nullptr)
		{
			SDL_DestroyMutex(mutex);
			mutex = nullptr;
		}
	}

	void Mutex::Lock()
	{
		if (SDL_LockMutex(mutex) < 0)
		{
			Debug::Print("SDL mutex lock failed");
		}
	}

	void Mutex::Unlock()
	{
		if (SDL_UnlockMutex(mutex) < 0)
		{
			Debug::Print("SDL mutex unlock failed");
		}
	}
}
