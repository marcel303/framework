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

#include "audioTypes.h"
#include "Debugging.h"
#include <SDL2/SDL.h>

#if !AUDIO_USE_SSE
	#warning "AUDIO_USE_SSE is set to 0. is this intended?"
#endif

AudioMutex_Shared::AudioMutex_Shared()
	: mutex(nullptr)
{
}

AudioMutex_Shared::AudioMutex_Shared(SDL_mutex * _mutex)
	: mutex(_mutex)
{
}
	
void AudioMutex_Shared::lock() const
{
	Assert(mutex != nullptr);
	const int result = SDL_LockMutex(mutex);
	Assert(result == 0);
	(void)result;
}

void AudioMutex_Shared::unlock() const
{
	Assert(mutex != nullptr);
	const int result = SDL_UnlockMutex(mutex);
	Assert(result == 0);
	(void)result;
}

//

AudioMutex::AudioMutex()
	: mutex(nullptr)
{
}

AudioMutex::~AudioMutex()
{
	Assert(mutex == nullptr);
}

void AudioMutex::init()
{
	Assert(mutex == nullptr);
	mutex = SDL_CreateMutex();
	Assert(mutex != nullptr);
}

void AudioMutex::shut()
{
	Assert(mutex != nullptr);
	if (mutex != nullptr)
	{
		SDL_DestroyMutex(mutex);
		mutex = nullptr;
	}
}

void AudioMutex::lock() const
{
	Assert(mutex != nullptr);
	const int result = SDL_LockMutex(mutex);
	Assert(result == 0);
	(void)result;
}

void AudioMutex::unlock() const
{
	Assert(mutex != nullptr);
	const int result = SDL_UnlockMutex(mutex);
	Assert(result == 0);
	(void)result;
}

//

AudioRNG::AudioRNG()
{
}

float AudioRNG::nextf(const float min, const float max)
{
	const float t = rand() / float(RAND_MAX);
	
	return min * t + max * (1.f - t);
}

double AudioRNG::nextd(const double min, const double max)
{
	const double t = rand() / double(RAND_MAX);
	
	return min * t + max * (1.0 - t);
}
