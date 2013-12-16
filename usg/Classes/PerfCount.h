#pragma once

#include "DeltaTimer.h"
#include "Log.h"

#define PC_MAX 40

#define PC_RENDER 0
#define PC_RENDER_MAKECURRENT 1
#define PC_RENDER_SETUP 2
#define PC_RENDER_PRESENT 3
#define PC_RENDER_BACKGROUND 4
#define PC_RENDER_SHADOWS 5
#define PC_RENDER_PRIMARY 6
#define PC_RENDER_PARTICLES 7
#define PC_RENDER_INTERFACE 8
#define PC_RENDER_FLUSH 9
#define PC_UPDATE 10
#define PC_UPDATE_SB_CLEAR 11
#define PC_UPDATE_SB_DRAW 12
#define PC_UPDATE_SB_DRAW_SCAN 13
#define PC_UPDATE_SB_DRAW_FILL 14
#define PC_UPDATE_LOGIC 15
#define PC_UPDATE_HITTEST 16
#define PC_UPDATE_POST 17

enum PerfType
{
	PerfType_Undefined,
	PerfType_Time,
	PerfType_Accum,
	PerfType_Count
};

class PerfValue
{
public:
	PerfValue();
	
	void Setup(const char* name, PerfType type);
	
	void DBG_Show(LogCtx* log);
	
	const char* m_Name;
	PerfType m_Type;
	float m_Time;
	float m_Value;
	int m_Count;
};

class PerfCount
{
public:
	PerfCount();
	
	void Create(int pc, const char* name, PerfType type);
	
	void DBG_Show();
	
	//
	
	void Set_Time(int pc, float time);
	void Set_Count(int pc, int count);
	void Increment_Count(int pc, int count);
	void Accumulate(int pc, float value);
	
	PerfValue m_Values[PC_MAX];
	LogCtx m_Log;
};

extern PerfCount g_PerfCount;

class PerfTimer
{
public:
	PerfTimer(int pc);
	~PerfTimer();
	
private:
	DeltaTimer m_Timer;
	int m_PC;
};
