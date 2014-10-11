#pragma once

// when set to 1, use VirtualAlloc to allocate the EGA memory, making it
// easier to detect out of range memory accesses and memory corruption
#define PROTECT_DISPLAY_BUFFER 1

// size of a single EGA bit plane (in bytes)
#define DISPLAY_BUFFER_SIZE (64*1024)

#if PROTECT_DISPLAY_BUFFER
	extern unsigned char * g0xA000[4];
#else
	extern unsigned char g0xA000[4][DISPLAY_BUFFER_SIZE];
#endif

#define AUDIO_SAMPLE_RATE 44100

void SYS_Init(int tickrate, int displaySx, int displaySy, int fullscreen);
void SYS_SetPalette(char * palette);
void SYS_Present();
void SYS_Update();
void SYS_PlaySound(unsigned short sound);
void SYS_StopSound();
