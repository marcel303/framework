#include "allegro2-mutex.h"
#include <mutex>

AllegroMutex::AllegroMutex()
{
	mutex = new std::recursive_mutex();
}

AllegroMutex::~AllegroMutex()
{
	auto * std_mutex = (std::recursive_mutex*)mutex;

	delete std_mutex;
}

void AllegroMutex::lock()
{
	auto * std_mutex = (std::recursive_mutex*)mutex;

	std_mutex->lock();	
}

void AllegroMutex::unlock()
{
	auto * std_mutex = (std::recursive_mutex*)mutex;

	std_mutex->unlock();
}
