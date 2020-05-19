#include "Debugging.h"
#include "Mutex.h"
#include <mutex>

Mutex::~Mutex()
{
	AssertMsg(opaque == nullptr, "free() not called before object was destroyed", 0);
}

void Mutex::alloc()
{
	Assert(opaque == nullptr);
	opaque = new std::recursive_mutex();
}

void Mutex::free()
{
	if (opaque != nullptr)
	{
		auto * mutex = (std::recursive_mutex*)opaque;
		
		delete mutex;
		mutex = nullptr;
		
		opaque = nullptr;
	}
}

void Mutex::lock() const
{
	auto * mutex = (std::recursive_mutex*)opaque;
	mutex->lock();
}

void Mutex::unlock() const
{
	auto * mutex = (std::recursive_mutex*)opaque;
	mutex->unlock();
}
