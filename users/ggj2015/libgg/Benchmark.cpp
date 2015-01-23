#include "Benchmark.h"
#include "Timer.h"

int Benchmark::m_Indent = 0;

Benchmark::Benchmark(const char* name)
{
#if ENABLE_BENCHMARK == 1
	m_Name = name;
	
	char indentString[10];
	CreateIndentString(indentString);
	
	LOG_INF("%sBenchmark: %s [begin]", indentString, m_Name);
	
	m_Indent++;
	
	m_Timer.Initialize(&g_TimerRT);
	m_Timer.Start();
#endif
}

Benchmark::~Benchmark()
{
#if ENABLE_BENCHMARK == 1
	float delta = m_Timer.Delta_get();
	
	m_Indent--;
	
	char indentString[10];
	CreateIndentString(indentString);
	
	LOG_INF("%sBenchmark: %s: %f sec", indentString, m_Name, delta);
#endif
}

void Benchmark::CreateIndentString(char* out_String)
{
	int indent = m_Indent;
	
	for (int i = 0; i < indent; ++i)
		out_String[i] = '\t';
	out_String[indent] = 0;
}
