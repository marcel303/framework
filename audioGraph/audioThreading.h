/*
	Copyright (C) 2017 Marcel Smit
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

#include <stdint.h>

struct SDL_mutex;

/**
 * AudioMutex_Shared contains a shared reference to a SDL mutex.
 * It provides lock and unlock methods which will lock and unlock the mutex, as well as perform some additional checks in debug mode.
 */
struct AudioMutex_Shared
{
	SDL_mutex * mutex;
	
	AudioMutex_Shared();
	AudioMutex_Shared(SDL_mutex * mutex);
	
	void lock() const; ///< Locks the mutex. Asserts the lock operation succeeds in debug mode.
	void unlock() const; ///< Unlocks the mutex. Assers the unlock operation succeeds in debug mode.
};

/**
 * AudioMutex provides an interface for locking and unlocking a SDL mutex.
 * The AudioMutex must first be initialized using init(), which will create the mutex.
 * shut() must explicitly be called to free the mutex again.
 */
struct AudioMutex
{
	SDL_mutex * mutex; ///< Reference to the SDL mutex owned by this AudioMutex. Exposed here for convience when direct usage of the SDL mutex is needed.
	
	AudioMutex();
	~AudioMutex(); ///< Asserts shut() has been called before the AudioMutex leaves scope.
	
	void init(); ///< Creates the SDL mutex.
	void shut(); ///< Destroys the SDL mutex.
	
	void lock() const; ///< Locks the mutex. Asserts the lock operation succeeds in debug mode.
	void unlock() const; ///< Unlocks the mutex. Assers the unlock operation succeeds in debug mode.

	void debugCheckIsLocked(); ///< Asserts the mutex has been locked in debug mode.
};

/**
 * Helper object which can be locked to the current thread (using initThreadId), and which can be used to check if the current thread is equal to the thread previously locked to, using checkThreadId.
 */
struct AudioThreadId
{
	int64_t id; ///< The thread id this object has been assigned. Set to -1 by default.
	
	AudioThreadId();
	
	void initThreadId(); ///< Assigns the id of the current thread to 'id'.
	
	bool checkThreadId() const; ///< Asserts the current thread is equal to the thread the object was previously assigned to.
};
