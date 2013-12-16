#include "log.h"

#define VA_SPRINTF(text, out, last_arg) \
		va_list va; \
		char out[1000]; \
		va_start(va, last_arg); \
		vsprintf(out, text, va); \
		va_end(va);

#if DEBUG

void LogMgr::WriteLine(LogLevel level, const char* text, ...)
{
	VA_SPRINTF(text, temp, text);

#if 0 // todo: use NSLog
	iprintf("[%s]", LevelToString(level));
	iprintf(" ");
	iprintf("%s", temp);
	iprintf("\n");
#endif
	
	fprintf(stderr, "[%s]", LevelToString(level));
	fprintf(stderr, " ");
	fprintf(stderr, "%s", temp);
	fprintf(stderr, "\n");
}

#endif

const char* LogMgr::LevelToString(LogLevel level)
{
	switch (level)
	{
	case LogLevel_Debug:
		return "DD";
		break;
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

#if DEBUG

void LogCtx::WriteLine(LogLevel level, const char* text, ...) const
{
	VA_SPRINTF(text, temp, text);

	LogMgr::WriteLine(level, "%s: %s", m_Name, temp);
}

#endif

