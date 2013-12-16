#include <AudioToolbox/AudioFormat.h>
#include <AudioToolbox/AudioToolbox.h>
#include "AudioOps.h"
#include "Exception.h"

namespace AudioOps
{
	static uint32_t AudioFile_GetSize(AudioFileID id)
	{
		UInt64 size = 0;
		UInt32 propSize = sizeof(size);
		
		OSStatus retval = AudioFileGetProperty(id, kAudioFilePropertyAudioDataByteCount, &propSize, &size);
		
		if (retval != 0)
		{
			throw ExceptionVA("unable to get audio size: %d", retval);
		}
		
		return (uint32_t)size;
	}

	static AudioStreamBasicDescription AudioFile_GetDescription(AudioFileID id)
	{
		AudioStreamBasicDescription dataFormat;

		UInt32 size = sizeof(dataFormat);
		
		OSStatus retval = AudioFileGetProperty(id, kAudioFilePropertyDataFormat, &size, &dataFormat);
		
		if (retval != 0)
		{
			throw ExceptionVA("unable to get audio description: %d", retval);
		}
		
		return dataFormat;
	}
	
	void LoadAudioFile(const char* fileName, int* out_ByteCount, uint8_t** out_Bytes, AudioStreamBasicDescription* out_Desc)
	{
		AudioFileID id;
		OSStatus retval;
		
		NSURL* url = [NSURL fileURLWithPath:path];
 
		retval = AudioFileOpenURL((CFURLRef)url, fsRdPerm, 0, &id);
		
		if (retval != 0)
		{
			throw ExceptionVA("unable to open audio file: %d", retval);
		}
		
		UInt32 byteCount = AudioFile_GetSize(id);
		uint8_t* bytes = new uint8_t[byteCount];
		const AudioStreamBasicDescription desc = AudioFile_GetDescription(id);
		 
		retval = AudioFileReadBytes(id, false, 0, &byteCount, bytes);
		
		if (retval != 0)
		{
			throw ExceptionVA("unable to read audio data: %d", retval);
		}
		
		retval = AudioFileClose(id);
		
		if (retval != 0)
		{
			throw ExceptionVA("unable to close audio file: %d", retval);
		}
		
		*out_ByteCount = byteCount;
		*out_Bytes = bytes;
		*out_Desc = desc;
	}
}
