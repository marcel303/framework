#ifdef DEBUG

#include "PsdLog.h"
#include <stdarg.h>
#include <stdio.h>

void PSD_LOG_DBG(const char* text, ...)
{
    va_list va;
    va_start(va, text);
    char temp[1024];
    vsprintf(temp, text, va);
    va_end(va);
    
    printf("%s", temp);
    printf("\n");
}

#endif
