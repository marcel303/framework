#pragma once

#define DEBUG 1

#if DEBUG
void checkErrorGL();
#else
#define checkErrorGL do { } while (false)
#endif

#if DEBUG
void logDebug(const char * format, ...);
#else
#define logDebug(...) do { } while (false)
#endif
void logWarning(const char * format, ...);
void logError(const char * format, ...);
