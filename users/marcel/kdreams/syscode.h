#pragma once

#define PROTECT_DISPLAY_BUFFER 1

#define DISPLAY_BUFFER_SIZE (64*1024) // todo : figure out exact size this needs to be + layout of buffers in mem

#if PROTECT_DISPLAY_BUFFER
	extern unsigned char * g0xA000[4];
#else
	extern unsigned char g0xA000[4][DISPLAY_BUFFER_SIZE];
#endif

void SYS_Init();
void SYS_Present();
void SYS_Update();
void SYS_PlaySound(const struct SampledSound *sample);
void SYS_StopSound();
