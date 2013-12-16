//#include <iostream>
#include "Log.h"
#include "LogUdp.h"

#ifndef DEPLOYMENT

void LogMgr::_WriteLine(LogLevel level, const char* text, ...)
{
#if !defined(DEBUG)
	if (level < LogLevel_Info)
		return;
#endif
	
	VA_SPRINTF(text, temp, text);
	
#ifdef PSP
	char temp2[VA_BUFSIZE];
	sprintf(temp2, "[%s] %s\n", LevelToString(level), temp);
	printf(temp2);
#else
	fprintf(stderr, "[%s] %s\n", LevelToString(level), temp);
#endif

	//LogUdp::Init();
	//LogUdp::Send(temp);
}

void LogMgr::_WriteLineNA(LogLevel level, const char* name, const char* text)
{
#if !defined(DEBUG)
	if (level < LogLevel_Info)
		return;
#endif
	
#ifdef PSP
	fprintf(stderr, "[%s] %s: %s\n", LevelToString(level), name, text);
#else
	fprintf(stderr, "[%s] %s: %s\n", LevelToString(level), name, text);
#endif

	//LogUdp::Init();
	//LogUdp::Send(temp);
}

#endif

const char* LogMgr::LevelToString(LogLevel level)
{
	switch (level)
	{
	case LogLevel_Debug:
		return "DD";
	case LogLevel_Info:
		return "II";
	case LogLevel_Warning:
		return "WW";
	case LogLevel_Error:
		return "EE";
	case LogLevel_Critical:
		return "CC";
	default:
		return "??";
	}
}

LogCtx::LogCtx()
{
	m_Name = "Global";
}

LogCtx::LogCtx(const char* name)
{
	m_Name = name;
}

#ifndef DEPLOYMENT

void LogCtx::WriteLine(LogLevel level, const char* text, ...) const
{
#ifndef DEBUG
	if (level < LogLevel_Info)
		return;
#endif
	
	VA_SPRINTF(text, temp, text);

	LOG(level, "%s: %s", m_Name, temp);
}

void LogCtx::WriteLineNA(LogLevel level, const char* text) const
{
#ifndef DEBUG
	if (level < LogLevel_Info)
		return;
#endif

	LOG_NA(level, m_Name, text);
}

#endif
