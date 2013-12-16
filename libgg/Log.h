#pragma once

#include <stdarg.h>
#include <stdio.h>

#define VA_BUFSIZE 4096

#define VA_SPRINTF(text, out, last_arg) \
		va_list va; \
		va_start(va, last_arg); \
		char out[VA_BUFSIZE]; \
		vsprintf(out, text, va); \
		va_end(va);

#define VA_SPRINTF_STATIC(text, out, last_arg) \
		va_list va; \
		va_start(va, last_arg); \
		static char out[VA_BUFSIZE]; \
		vsprintf(out, text, va); \
		va_end(va);

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
#ifndef DEPLOYMENT
	static void _WriteLine(LogLevel level, const char* text, ...);
	static void _WriteLineNA(LogLevel level, const char* name, const char* text);
#else
	static inline void _WriteLine(LogLevel level, const char* text, ...)
	{
	}
	static inline void _WriteLineNA(LogLevel level, const char* text)
	{
	}
#endif

	static const char* LevelToString(LogLevel level);
};

#ifndef DEPLOYMENT
#define LOG    LogMgr::_WriteLine
#define LOG_NA LogMgr::_WriteLineNA
#else
#define LOG(level, text, ...)
#define LOG_NA(level, text, ...)
#endif

#define LOG_ERR(text, ...) LOG(LogLevel_Error, text, __VA_ARGS__)
#define LOG_WRN(text, ...) LOG(LogLevel_Warning, text, __VA_ARGS__)
#define LOG_WRN(text, ...) LOG(LogLevel_Warning, text, __VA_ARGS__)
#define LOG_INF(text, ...) LOG(LogLevel_Info, text, __VA_ARGS__)
#define LOG_DBG(text, ...) LOG(LogLevel_Debug, text, __VA_ARGS__)

class LogCtx
{
public:
	LogCtx();
	LogCtx(const char* name);

#ifndef DEPLOYMENT
	void WriteLine(LogLevel level, const char* text, ...) const;
	void WriteLineNA(LogLevel level, const char* text) const;
#else
	inline void WriteLine(LogLevel level, const char* text, ...) const
	{
	}
	inline void WriteLineNA(LogLevel level, const char* text) const
	{
	}
#endif
	
	const char* m_Name;
};
