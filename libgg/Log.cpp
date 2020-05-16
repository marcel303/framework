#include "Log.h"

#ifndef DEPLOYMENT

#if defined(ANDROID)
	#include <android/log.h>

	static int LevelToAdroidPrio(LogLevel level)
	{
		switch (level)
		{
		case LogLevel_Debug:
			return ANDROID_LOG_VERBOSE;
		case LogLevel_Info:
			return ANDROID_LOG_INFO;
		case LogLevel_Warning:
			return ANDROID_LOG_WARN;
		case LogLevel_Error:
			return ANDROID_LOG_ERROR;
		case LogLevel_Critical:
			return ANDROID_LOG_FATAL;
		}

		return ANDROID_LOG_VERBOSE;
	}
#endif

void LogMgr::_WriteLine(LogLevel level, const char* text, ...)
{
#if !defined(DEBUG)
	if (level < LogLevel_Info)
		return;
#endif
	
	VA_SPRINTF(text, temp, text);

#if defined(PSP)
	char temp2[VA_BUFSIZE];
	sprintf(temp2, "[%s] %s\n", LevelToString(level), temp);
	printf("%s", temp2);
#elif defined(ANDROID)
	__android_log_write(LevelToAdroidPrio(level), "GG", temp);
#else
	fprintf(stderr, "[%s] %s\n", LevelToString(level), temp);
#endif
}

void LogMgr::_WriteLineNA(LogLevel level, const char* name, const char* text)
{
#if !defined(DEBUG)
	if (level < LogLevel_Info)
		return;
#endif

#if defined(PSP)
	printf("%s", text);
#elif defined(ANDROID)
	__android_log_write(LevelToAdroidPrio(level), "GG", text);
#else
	fprintf(stderr, "[%s] %s: %s\n", LevelToString(level), name, text);
#endif
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
