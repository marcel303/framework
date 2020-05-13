#pragma once

#define DEBUG 1

class Framework
{
public:
    void process();

    float time = 0.f;
};

extern Framework framework;

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
void logError(const char * format, ...);
