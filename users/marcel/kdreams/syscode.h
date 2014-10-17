#pragma once

// when set to 1, use VirtualAlloc to allocate the EGA memory, making it
// easier to detect out of range memory accesses and memory corruption
#ifdef WIN32
	#define PROTECT_DISPLAY_BUFFER 1
#else
	#define PROTECT_DISPLAY_BUFFER 0
#endif

// size of a single EGA bit plane (in bytes)
#define DISPLAY_BUFFER_SIZE (64*1024)

#if PROTECT_DISPLAY_BUFFER
	extern unsigned char * g0xA000[4];
#else
	extern unsigned char g0xA000[4][DISPLAY_BUFFER_SIZE];
#endif

#define AUDIO_SAMPLE_RATE 48000 // Adlib OPL = 49716 Hz

#define k_ADLIB_EMU_DOSBOX_OPL	0
#define k_ADLIB_EMU_MAME		1
#define ADLIB_EMU k_ADLIB_EMU_DOSBOX_OPL

void SYS_Init(int tickrate, int displaySx, int displaySy, int fullscreen, int fixedAspect, int useOpengl);
void SYS_SetPalette(char * palette);
void SYS_Present();
void SYS_Update();
void SYS_PlaySound(unsigned short sound);
void SYS_StopSound();
