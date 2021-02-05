/*
	Copyright (C) 2020 Marcel Smit
	marcel303@gmail.com
	https://www.facebook.com/marcel.smit981

	Permission is hereby granted, free of charge, to any person
	obtaining a copy of this software and associated documentation
	files (the "Software"), to deal in the Software without
	restriction, including without limitation the rights to use,
	copy, modify, merge, publish, distribute, sublicense, and/or
	sell copies of the Software, and to permit persons to whom the
	Software is furnished to do so, subject to the following
	conditions:

	The above copyright notice and this permission notice shall be
	included in all copies or substantial portions of the Software.

	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
	EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
	OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
	NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
	HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
	WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
	FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
	OTHER DEALINGS IN THE SOFTWARE.
*/

#include "SoundIO.h"

#include "audiostream/WaveLoader.h"

#include "../internal_filereader.h"

#include "Log.h"

SoundData * loadSound_WAV(const char * filename)
{
	FileReader r;
	
	if (!r.open(filename, false))
	{
		LOG_ERR("failed to open %s", filename);
		return nullptr;
	}
	
	WaveHeadersReader headers;
	
	if (!headers.read(r.file))
	{
		return nullptr;
	}
	
	if (headers.hasFmt == false)
	{
		LOG_ERR("missing FMT chunk. cannot load WAVE data when we don't know the format yet");
		return nullptr;
	}

	if (headers.hasData == false)
	{
		LOG_ERR("missing DATA chunk");
		return nullptr;
	}
	
	if (!r.seek(headers.dataOffset))
	{
		LOG_ERR("failed to seek to WAVE data");
		return nullptr;
	}
	
	uint8_t * bytes = new uint8_t[headers.dataLength];
	int numBytes = headers.dataLength;

	if (!r.read(bytes, numBytes))
	{
		LOG_ERR("failed to load WAVE data");
		delete [] bytes;
		return nullptr;
	}

	// convert data if necessary

	if (headers.fmtCompressionType == WAVE_FORMAT_PCM)
	{
		if (headers.fmtBitDepth == 8)
		{
			// for 8 bit data the integers are unsigned. convert them to signed here
			
			const uint8_t * srcValues = bytes;
			int8_t * dstValues = (int8_t*)bytes;
			const int numValues = numBytes;
			
			for (int i = 0; i < numValues; ++i)
			{
				const int value = int(srcValues[i]) - 128;
				
				dstValues[i] = value;
			}
		}
		else if (headers.fmtBitDepth == 16)
		{
			// 16 bit data is already signed. no conversion needed
		}
		else if (headers.fmtBitDepth == 24)
		{
			const int sampleCount = numBytes / 3;
			float * samplesData = new float[sampleCount];
			
			for (int i = 0; i < sampleCount; ++i)
			{
				int32_t value =
					(bytes[i * 3 + 0] << 8) |
					(bytes[i * 3 + 1] << 16) |
					(bytes[i * 3 + 2] << 24);
				
				// perform a shift right to sign-extend the 24 bit number (which we packed into the top-most 24 bits of a 32 bits number)
				value >>= 8;
				
				samplesData[i] = value / float(1 << 23);
			}
			
			delete [] bytes;
			bytes = nullptr;
			
			bytes = (uint8_t*)samplesData;
			
			headers.fmtBitDepth = 32;
			numBytes = numBytes * 4 / 3;
		}
		else if (headers.fmtBitDepth == 32)
		{
			const int32_t * srcValues = (int32_t*)bytes;
			float * dstValues = (float*)bytes;
			const int numValues = numBytes / 4;
			
			for (int i = 0; i < numValues; ++i)
			{
				dstValues[i] = float(srcValues[i] / double(1 << 31));
			}
		}
	}
	else if (headers.fmtCompressionType == WAVE_FORMAT_IEEE_FLOAT)
	{
		if (headers.fmtBitDepth == 32)
		{
			// no conversion is needed
		}
		else
		{
			LOG_ERR("only 32 bit IEEE float is supported");
			delete [] bytes;
			return nullptr;
		}
	}
	else
	{
		LOG_ERR("unknown WAVE data format");
		delete [] bytes;
		return nullptr;
	}
	
	SoundData * soundData = new SoundData();
	soundData->channelSize = headers.fmtBitDepth / 8;
	soundData->channelCount = headers.fmtChannelCount;
	soundData->sampleCount = numBytes / (headers.fmtBitDepth / 8 * headers.fmtChannelCount);
	soundData->sampleRate = headers.fmtSampleRate;
	soundData->sampleData = bytes;
	
	return soundData;
}
