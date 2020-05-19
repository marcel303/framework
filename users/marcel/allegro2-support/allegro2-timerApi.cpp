#include "allegro2-mutex.h"
#include "allegro2-timerApi.h"
#include <assert.h>
#include <atomic>
#include <thread>

#define Assert assert

#define Verify(x) do { const bool y = x; (void)y; Assert(y); } while (false)

typedef void (*TimerProc)();

struct AllegroTimerReg
{
	AllegroTimerReg * next;
	AllegroTimerApi * owner;
	
	void (*proc)(void * data);
	void * data;
	std::atomic<bool> stop;
	int delay;
	int delayInMicroseconds;
	
	int sampleTime; // when processing manually
	std::thread * thread; // when using threaded processing
};

//

#ifndef WIN32
	#include <time.h>
	#include <unistd.h>
#endif

static int TimerThreadProc(void * obj)
{
	AllegroTimerReg * r = (AllegroTimerReg*)obj;
	
#ifdef WIN32
	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGH);
#else
    const pthread_t thread = pthread_self();
#ifdef MACOS
#error
	pthread_setname_np("Allegro timer");
#endif
	
    struct sched_param sched;
    int policy;
	int retval = pthread_getschedparam(thread, &policy, &sched);
	assert(retval == 0);
	const int min_priority = sched_get_priority_min(policy);
	const int max_priority = sched_get_priority_max(policy);
	sched.sched_priority = min_priority + (max_priority - min_priority) * 4/5;
	retval = pthread_setschedparam(thread, policy, &sched);
	assert(retval == 0);
#endif
	
#ifndef WIN32
	struct timespec start, end;
	clock_gettime(CLOCK_MONOTONIC_RAW, &start);
#endif

	while (r->stop == false)
	{
		r->proc(r->data);
		
	#ifndef WIN32
		clock_gettime(CLOCK_MONOTONIC_RAW, &end);
		
		const int64_t delta_us = (end.tv_sec - start.tv_sec) * 1000000 + (end.tv_nsec - start.tv_nsec) / 1000;
		
		if (delta_us < r->delay)
		{
			const int64_t todo_us = r->delay - delta_us;
			
			usleep(todo_us);
		}
		else
		{
			printf("no wait..\n");
		}
		
		clock_gettime(CLOCK_MONOTONIC_RAW, &start);
	#endif
	}
	
	return 0;
}

//

AllegroTimerApi::AllegroTimerApi(const Mode in_mode)
	: mutex(nullptr)
	, mode(in_mode)
{
	mutex = new AllegroMutex();
}

AllegroTimerApi::~AllegroTimerApi()
{
	delete mutex;
	mutex = nullptr;
}

void AllegroTimerApi::handle_int(void * data)
{
	TimerProc proc = (TimerProc)data;
	
	proc();
}

void AllegroTimerApi::install_int_ex(void (*proc)(), int speed)
{
	install_int_ex2(handle_int, speed, (void*)proc);
}

void AllegroTimerApi::install_int_ex2(void (*proc)(void * data), int speed, void * data)
{
	bool preexisting = false;
	
	lock();
	{
		for (auto r = timerRegs; r != nullptr; r = timerRegs->next)
		{
			if (r->proc == proc && r->data == data)
			{
				r->delay = speed;
				r->delayInMicroseconds = int64_t(speed);
				preexisting = true;
				break;
			}
		}
	}
	unlock();
	
	if (preexisting)
		return;
	
	AllegroTimerReg * r = new AllegroTimerReg();
	
	r->owner = this;
	r->proc = proc;
	r->data = data;
	r->stop = false;
	r->delay = speed;
	r->delayInMicroseconds = int64_t(speed);
	
	r->sampleTime = 0;
	r->thread = nullptr;
	
	lock();
	{
		r->next = timerRegs;
		timerRegs = r;
	}
	unlock();
	
	if (mode == kMode_Threaded)
	{
		r->thread = new std::thread(TimerThreadProc, r);
	}
}

void AllegroTimerApi::remove_int(void (*proc)())
{
	remove_int2(handle_int, (void*)proc);
}

void AllegroTimerApi::remove_int2(void (*proc)(void * data), void * data)
{
	lock();
	
	AllegroTimerReg ** r = &timerRegs;
	
	while ((*r) != nullptr)
	{
		AllegroTimerReg * t = *r;
		
		if ((*r)->proc == proc && (*r)->data == data)
		{
			*r = t->next;
			
			//
			
			if (t->thread != nullptr)
			{
				unlock(); // temporarily give up our lock, to make sure the work executing on the thread has access to the timer api too
				{
					t->stop = true;
				
					t->thread->join();
					
					delete t->thread;
					t->thread = nullptr;
				}
				lock();
			}
			
			delete t;
			t = nullptr;
		}
		else
		{
			r = &t->next;
		}
	}
	
	unlock();
}

void AllegroTimerApi::lock()
{
	mutex->lock();
}

void AllegroTimerApi::unlock()
{
	mutex->unlock();
}

void AllegroTimerApi::processInterrupts(const int numMicroseconds)
{
	Assert(mode == kMode_Manual);
	if (mode != kMode_Manual)
		return;
	
	lock();
	{
		for (AllegroTimerReg * r = timerRegs; r != nullptr; r = r->next)
		{
			if (r->delayInMicroseconds == 0) // infinity
				continue;
			
			r->sampleTime += numMicroseconds;
			
			while (r->sampleTime >= r->delayInMicroseconds)
			{
				r->sampleTime -= r->delayInMicroseconds;
				
				r->proc(r->data);
			}
		}
	}
	unlock();
}
