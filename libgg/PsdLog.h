#pragma once

#ifdef DEBUG
void PSD_LOG_DBG(const char* text, ...);
#else
#define PSD_LOG_DBG(...) { }
#endif
