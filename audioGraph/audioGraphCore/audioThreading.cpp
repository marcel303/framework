/*
	Copyright (C) 2020 Marcel Smit
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

#include "audioThreading.h"
#include "Debugging.h"
#include "Multicore/Mutex.h"

AudioMutex_Shared::AudioMutex_Shared()
	: mutex(nullptr)
{
}

AudioMutex_Shared::AudioMutex_Shared(Mutex * in_mutex)
	: mutex(in_mutex)
{
}
	
void AudioMutex_Shared::lock() const
{
	Assert(mutex != nullptr);
	mutex->lock();
}

void AudioMutex_Shared::unlock() const
{
	Assert(mutex != nullptr);
	mutex->unlock();
}

//

AudioMutex::AudioMutex()
	: mutex()
{
}

AudioMutex::~AudioMutex()
{
}

void AudioMutex::init()
{
	mutex.alloc();
}

void AudioMutex::shut()
{
	mutex.free();
}

void AudioMutex::lock() const
{
	mutex.lock();
}

void AudioMutex::unlock() const
{
	mutex.unlock();
}

// -- audio thread id

// note : we cannot get the thread id as an integer representation directly, so we are forced to use std::hash to generate an integer. this doesn't guarantee unique thread id's, but it's better than nothing I guess

#include <functional> // std::hash
#include <thread>

AudioThreadId::AudioThreadId()
	: id(-1)
{
}

void AudioThreadId::setThreadId()
{
	id = (int64_t)std::hash<std::thread::id>()(std::this_thread::get_id());
}

void AudioThreadId::clearThreadId()
{
	id = -1;
}

bool AudioThreadId::checkThreadId() const
{
	return (int64_t)std::hash<std::thread::id>()(std::this_thread::get_id()) == id;
}
