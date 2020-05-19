#include "allegro2-mutex.h"
#include <mutex>

AllegroMutex::AllegroMutex()
{
	mutex = new std::mutex();
}

AllegroMutex::~AllegroMutex()
{
	auto * std_mutex = (std::mutex*)mutex;

	delete std_mutex;
}

void AllegroMutex::lock()
{
	auto * std_mutex = (std::mutex*)mutex;

	std_mutex->lock();	
}

void AllegroMutex::unlock()
{
	auto * std_mutex = (std::mutex*)mutex;

	std_mutex->unlock();
}
