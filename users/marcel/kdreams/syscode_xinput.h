#pragma once

#ifdef WIN32
	#define BUILD_WITH_XINPUT 1
#else
	#define BUILD_WITH_XINPUT 0
#endif

#if BUILD_WITH_XINPUT
extern int SYS_PollXInput(unsigned char index, float * out_x, float * out_y, unsigned short * out_buttons);
#else
static int SYS_PollXInput(unsigned char index, float * out_x, float * out_y, unsigned short * out_buttons) { return 0; }
#endif
