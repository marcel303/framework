#pragma once

#include <SDL2/SDL.h> // fixme : abstract away

namespace MP
{
	class Mutex
	{
		SDL_mutex * mutex;

	public:
		Mutex();
		~Mutex();

		void Lock();
		void Unlock();
	};
}
