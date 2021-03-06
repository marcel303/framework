#pragma once

#define BPM_TO_TIMER(x) (x == 0 ? 0 : ((1000000 * 60) / (x)))

//

struct AllegroMutex;

struct AllegroTimerReg;

struct AllegroTimerApi
{
	enum Mode
	{
		kMode_Threaded,
		kMode_Manual
	};
	
	AllegroTimerReg * timerRegs = nullptr;
	
	AllegroMutex * mutex = nullptr;
	
	Mode mode;
	
	AllegroTimerApi(const Mode in_mode);
	~AllegroTimerApi();
	
	static void handle_int(void * data);
	
	void install_int_ex(void (*proc)(), int speed);
	void install_int_ex2(void (*proc)(void * data), int speed, void * data);
	void remove_int(void (*proc)());
	void remove_int2(void (*proc)(void * data), void * data);
	
	void lock();
	void unlock();
	
	void processInterrupts(const int numMicroseconds);
};
