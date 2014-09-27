#pragma once

#define DISPLAY_BUFFER_SIZE (64*1024*4) // todo : figure out exact size this needs to be + layout of buffers in mem
//#define DISPLAY_BUFFER_SIZE (40*384*3)

extern unsigned char g0xA000[4][DISPLAY_BUFFER_SIZE];

void SYS_Init();
void SYS_Present();
void SYS_Update();
