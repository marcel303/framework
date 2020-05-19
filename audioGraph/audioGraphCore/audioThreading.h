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

#pragma once

#include "Multicore/Mutex.h"
#include <stdint.h>

struct AudioMutexBase
{
	virtual ~AudioMutexBase() { }

	virtual void lock() const = 0;
	virtual void unlock() const = 0;
};

/**
 * AudioMutex_Shared contains a shared reference to a libgg mutex.
 * It provides lock and unlock methods which will lock and unlock the mutex.
 */
struct AudioMutex_Shared : AudioMutexBase
{
	Mutex * mutex;
	
	AudioMutex_Shared();
	AudioMutex_Shared(Mutex * mutex);
	
	virtual void lock() const override; ///< Locks the mutex. Asserts the lock operation succeeds in debug mode.
	virtual void unlock() const override; ///< Unlocks the mutex. Assers the unlock operation succeeds in debug mode.
};

/**
 * AudioMutex provides an interface for locking and unlocking a mutex.
 * The AudioMutex must first be initialized using init(), which will create the mutex.
 * shut() must explicitly be called to free the mutex again.
 */
struct AudioMutex : AudioMutexBase
{
	Mutex mutex; ///< Reference to the libgg mutex owned by this AudioMutex. Exposed here for convience when direct usage of the mutex is needed.
	
	AudioMutex();
	virtual ~AudioMutex() override; ///< Asserts shut() has been called before the AudioMutex leaves scope.
	
	void init(); ///< Creates the underlying mutex.
	void shut(); ///< Destroys the underlying mutex.
	
	virtual void lock() const override; ///< Locks the mutex. Asserts the lock operation succeeds in debug mode.
	virtual void unlock() const override; ///< Unlocks the mutex. Assers the unlock operation succeeds in debug mode.

	void debugCheckIsLocked(); ///< Asserts the mutex has been locked in debug mode.
};

/**
 * Helper object which can be locked to the current thread (using setThreadId), and which can be used to check if the current thread is equal to the thread previously locked to, using checkThreadId.
 */
struct AudioThreadId
{
	intptr_t id; ///< The thread id this object has been assigned. Set to -1 by default.
	
	AudioThreadId();
	
	void setThreadId();   ///< Assigns the id of the current thread to 'id'.
	void clearThreadId(); ///< Clears the thread id assignment.
	
	bool checkThreadId() const; ///< Asserts the current thread is equal to the thread the object was previously assigned to.
};
