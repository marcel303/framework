#pragma once

#include "DeltaTimer.h"
#include "Log.h"

#if defined(DEPLOYMENT)
	#define ENABLE_BENCHMARK 0
#else
	#define ENABLE_BENCHMARK 1
#endif

class Benchmark
{
public:
	Benchmark(const char* name);
	~Benchmark();
	
private:
	static void CreateIndentString(char* out_String);
	const char* m_Name;
	DeltaTimer m_Timer;
	static int m_Indent;
};
