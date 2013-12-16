#include "PerfCount.h"
#include "Timer.h"

//

PerfCount g_PerfCount;

//

PerfValue::PerfValue()
{
	m_Name = 0;
	m_Type = PerfType_Undefined;
	m_Time = 0.0f;
	m_Count = 0;
	m_Value = 0.0f;
}

void PerfValue::Setup(const char* name, PerfType type)
{
	m_Name = name;
	m_Type = type;
}

void PerfValue::DBG_Show(LogCtx* log)
{
	switch (m_Type)
	{
		case PerfType_Undefined:
			break;
			
		case PerfType_Time:
			log->WriteLine(LogLevel_Info, "[time] %s: [%f]", m_Name, m_Time);
			break;
			
		case PerfType_Count:
			log->WriteLine(LogLevel_Info, "[count] %s: [%d]", m_Name, m_Count);
			break;
			
		case PerfType_Accum:
			log->WriteLine(LogLevel_Info, "[accum] %s: [%f]", m_Name, m_Value);
			break;
	}
}

//

PerfCount::PerfCount()
{
	m_Log = LogCtx("Performance");
}

void PerfCount::Create(int pc, const char* name, PerfType type)
{
	m_Values[pc].Setup(name, type);
}

void PerfCount::DBG_Show()
{
	for (int i = 0; i < PC_MAX; ++i)
	{
		if (!m_Values[i].m_Name)
			continue;
		
		m_Values[i].DBG_Show(&m_Log);
	}
}

void PerfCount::Set_Time(int pc, float time)
{
	m_Values[pc].m_Time = time;
}

void PerfCount::Set_Count(int pc, int count)
{
	m_Values[pc].m_Count = count;
}

void PerfCount::Increment_Count(int pc, int count)
{
	m_Values[pc].m_Count += count;
}

void PerfCount::Accumulate(int pc, float value)
{
	m_Values[pc].m_Value += value;
}

//

PerfTimer::PerfTimer(int pc)
{
	m_Timer.Initialize(&g_TimerRT);
	m_Timer.Start();
	m_PC = pc;
}

PerfTimer::~PerfTimer()
{
	float time = m_Timer.Delta_get();
	
	if (g_PerfCount.m_Values[m_PC].m_Type == PerfType_Time)
		g_PerfCount.Set_Time(m_PC, time);
	if (g_PerfCount.m_Values[m_PC].m_Type == PerfType_Accum)
		g_PerfCount.Accumulate(m_PC, time);
}
