#pragma once

#include <stdarg.h>
#include <stdio.h>

enum LogLevel
{
	LogLevel_Debug,
	LogLevel_Info,
	LogLevel_Warning,
	LogLevel_Error,
	LogLevel_Critical
};

class LogMgr
{
public:
#if DEBUG
	static void WriteLine(LogLevel level, const char* text, ...);
#else
	static inline void WriteLine(LogLevel level, const char* text, ...)
	{
	}
#endif

	static const char* LevelToString(LogLevel level);
};

class LogCtx
{
public:
	LogCtx();
	LogCtx(const char* name);

#ifdef DEBUG
	void WriteLine(LogLevel level, const char* text, ...) const;
#else
	inline void WriteLine(LogLevel level, const char* text, ...) const
	{
	}
#endif
	
	const char* m_Name;
};
