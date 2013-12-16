#pragma once

#include <CoreAudio/CoreAudioTypes.h>

namespace AudioOps
{
	void LoadAudioFile(const char* fileName, int* out_ByteCount, uint8_t** out_Bytes, AudioStreamBasicDescription* out_Desc);
}
