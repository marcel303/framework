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

#ifdef __MACOS__

#include <atomic>
#include <string>
#include <vector>

struct SDL_cond;
struct SDL_mutex;
struct SDL_Thread;

struct DeepbeliefPrediction
{
	float certainty;
	std::string label;
};

struct DeepbeliefResult
{
	std::vector<DeepbeliefPrediction> predictions;
	
	double bufferCreationTime;
	double classificationTime;
	double sortTime;
	
	DeepbeliefResult()
		: predictions()
		, bufferCreationTime(0.0)
		, classificationTime(0.0)
		, sortTime(0.0)
	{
	}
};

struct Deepbelief
{
	struct Work
	{
		uint8_t * bytes;
		int sx;
		int sy;
		int numChannels;
		int pitch;
		float certaintyTreshold;

		~Work()
		{
			delete[] bytes;
			bytes = nullptr;
		}
	};

	struct State
	{
		bool stop;
		
		bool isDone;

		Work * work;
		
		bool hasResult;
		DeepbeliefResult result;
		
		//
		
		std::atomic_bool isInitialized;
		
		std::string networkFilename;
		void * network;
		
		SDL_Thread * thread;
		SDL_mutex * mutex;
		SDL_cond * workEvent;
		SDL_cond * doneEvent;
		
		State()
			: stop(false)
			, isDone(false)
			, work(nullptr)
			, hasResult(false)
			, result()
			, isInitialized(false)
			, networkFilename()
			, network(nullptr)
			, thread(nullptr)
			, mutex(nullptr)
			, workEvent(nullptr)
			, doneEvent(nullptr)
		{
		}
	};
	
	State * state;

	Deepbelief();
	~Deepbelief();

	void init(const char * networkFilename);
	bool doInit(const char * networkFilename);
	void shut();

	void process(const uint8_t * bytes, const int sx, const int sy, const int numChannels, const int pitch, const float certaintyTreshold);
	void wait(); // optional : wait for all of the processing to complete

	bool getResult(DeepbeliefResult & result);

	static int threadMainProc(void * arg);
	
	static bool threadInit(State * state);
	static void threadShut(State * state);
	static void threadMain(State * state);
};

#endif
