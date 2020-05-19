#pragma once

struct AllegroMutex
{
	void * mutex;

	AllegroMutex();
	~AllegroMutex();

	void lock();
	void unlock();
};
