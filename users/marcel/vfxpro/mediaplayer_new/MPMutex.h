#pragma once

struct SDL_mutex;

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
