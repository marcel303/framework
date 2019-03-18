#pragma once

#include <stdio.h>

#if defined(__GNUC__)
	#define sprintf_s(s, ss, f, ...) snprintf(s, ss, f, __VA_ARGS__)
	#define vsprintf_s(s, ss, f, a) vsnprintf(s, ss, f, a)
	#define strcpy_s(d, ds, s) strcpy(d, s)
	#define sscanf_s sscanf
#endif

struct JGMOD;
struct JGMOD_INFO;

void *jgmod_calloc (int size);

namespace jgmod
{
	int detect_m31 (const char *filename);
	int detect_m15 (const char *filename);
	int detect_s3m (const char *filename);
	int detect_xm (const char *filename);
	int detect_it(const char *filename);
	int detect_unreal_it (const char *filename);
	int detect_unreal_xm (const char *filename);
	int detect_unreal_s3m (const char *filename);
	JGMOD *load_m (const char *filename, int no_inst);
	JGMOD *load_s3m (const char *filename, const int start_offset, const bool fast_loading);
	JGMOD *load_it (const char *filename, int start_offset);
	JGMOD *load_xm (const char *filename, int start_offset);
	int get_it_info(const char *filename, int start_offset, JGMOD_INFO *ji);
	int get_s3m_info (const char *filename, const int start_offset, JGMOD_INFO * ji);
	int get_xm_info (const char *filename, int start_offset, JGMOD_INFO *ji);
	int get_m_info(const char *filename, int no_inst, JGMOD_INFO *ji);
}
