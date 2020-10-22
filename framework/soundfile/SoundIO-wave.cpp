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

#include "../internal_filereader.h"

#include "Log.h"

// RIFF/WAVE loader constants

#define WAVE_FORMAT_PCM 1
#define WAVE_FORMAT_IEEE_FLOAT 3
#define WAVE_FORMAT_EXTENSIBLE -2

//

enum Chunk
{
	kChunk_RIFF,
	kChunk_WAVE,
	kChunk_FMT,
	kChunk_DATA,
	kChunk_OTHER
};

static bool checkId(const char * id, const char * match)
{
	for (int i = 0; i < 4; ++i)
		if (tolower(id[i]) != tolower(match[i]))
			return false;
	
	return true;
}

static bool readChunk(FileReader & r, Chunk & chunk, int32_t & size)
{
	char id[4];
	
	if (!r.read(id, 4))
		return false;
	
	//LOG_DBG("RIFF chunk: %c%c%c%c", id[0], id[1], id[2], id[3]);
	
	chunk = kChunk_OTHER;
	size = 0;
	
	if (checkId(id, "WAVE"))
	{
		chunk = kChunk_WAVE;
		return true;
	}
	else if (checkId(id, "fmt "))
	{
		chunk = kChunk_FMT;
		return true;
	}
	else
	{
		if (checkId(id, "RIFF"))
			chunk = kChunk_RIFF;
		else if (checkId(id, "data"))
			chunk = kChunk_DATA;
		else if (checkId(id, "LIST") || checkId(id, "FLLR") || checkId(id, "JUNK") || checkId(id, "bext") || checkId(id, "fact"))
			chunk = kChunk_OTHER;
		else
		{
			LOG_ERR("unknown RIFF chunk: %c%c%c%c", id[0], id[1], id[2], id[3]);
			return false; // unknown
		}
		
		if (!r.read(size))
			return false;
		
		if (size < 0)
			return false;
		
		return true;
	}
}

struct WaveHeadersReader
{
	bool hasFmt = false;
	int32_t fmtLength;
	int16_t fmtCompressionType; // format code is a better name. 1 = PCM/integer, 2 = ADPCM, 3 = float, 7 = u-law
	int16_t fmtChannelCount;
	int32_t fmtSampleRate;
	int32_t fmtByteRate;
	int16_t fmtBlockAlign;
	int16_t fmtBitDepth;
	int16_t fmtExtraLength;
	
	bool hasData = false;
	int32_t dataOffset = 0;
	int32_t dataLength = 0;
	
	bool read(FileReader & r)
	{
		for (;;)
		{
			Chunk chunk;
			int32_t byteCount;
			
			if (!readChunk(r, chunk, byteCount))
				break;
			
			if (chunk == kChunk_RIFF || chunk == kChunk_WAVE)
			{
				// just process sub chunks
			}
			else if (chunk == kChunk_FMT)
			{
				bool ok = true;
				
				ok &= r.read(fmtLength);
				ok &= r.read(fmtCompressionType);
				ok &= r.read(fmtChannelCount);
				ok &= r.read(fmtSampleRate);
				ok &= r.read(fmtByteRate);
				ok &= r.read(fmtBlockAlign);
				ok &= r.read(fmtBitDepth);
				
				if (fmtCompressionType != 1)
					ok &= r.read(fmtExtraLength);
				else
					fmtExtraLength = 0;
				
				if (fmtCompressionType == WAVE_FORMAT_EXTENSIBLE)
				{
					// read WAVEFORMATEXTENSIBLE structure and change format accordingly
					
					int16_t numValidBits;
					int32_t channelMask;
					int32_t guidFormatTag;
					int8_t guidRemainder[12];
					
					ok &= r.read(numValidBits);
					ok &= r.read(channelMask);
					ok &= r.read(guidFormatTag);
					ok &= r.read(guidRemainder, 12);
					
					if (guidFormatTag == WAVE_FORMAT_PCM)
					{
						fmtCompressionType = WAVE_FORMAT_PCM;
					}
					else if (guidFormatTag == WAVE_FORMAT_IEEE_FLOAT)
					{
						fmtCompressionType = WAVE_FORMAT_IEEE_FLOAT;
					}
					else
					{
						LOG_ERR("unknown format found in WAVEFORMATEXTENSIBLE");
						ok = false;
					}
				}
				else
				{
					ok &= r.skip(fmtExtraLength);
				}
				
				if (!ok)
				{
					LOG_ERR("failed to read FMT chunk");
					return false;
				}
				
				if (fmtCompressionType != WAVE_FORMAT_PCM &&
					fmtCompressionType != WAVE_FORMAT_IEEE_FLOAT)
				{
					LOG_ERR("only PCM and IEEE float are supported. type: %d", fmtCompressionType);
					ok = false;
				}
				if (fmtChannelCount <= 0)
				{
					LOG_ERR("invalid channel count: %d", fmtChannelCount);
					ok = false;
				}
				if (fmtBitDepth != 8 && fmtBitDepth != 16 && fmtBitDepth != 24 && fmtBitDepth != 32)
				{
					LOG_ERR("bit depth not supported: %d", fmtBitDepth);
					ok = false;
				}
				
				if (!ok)
					return false;
			
				hasFmt = true;
			}
			else if (chunk == kChunk_DATA)
			{
				dataOffset = r.position();
				dataLength = byteCount;
				
				hasData = true;
				
				if (!r.skip(byteCount))
				{
					LOG_ERR("failed to seek until end of chunk");
					return false;
				}
			}
			else if (chunk == kChunk_OTHER)
			{
				//LOG_DBG("wave loader: skipping %d bytes of unknown chunk", byteCount);
				
				if (!r.skip(byteCount))
				{
					LOG_ERR("failed to seek until end of chunk");
					return false;
				}
			}
		}
		
		return true;
	}
};

SoundData * loadSound_WAV(const char * filename)
{
	FileReader r;
	
	if (!r.open(filename, false))
	{
		LOG_ERR("failed to open %s", filename);
		return nullptr;
	}
	
	WaveHeadersReader headers;
	
	if (!headers.read(r))
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
		LOG_ERR("missing DATA chunk. cannot load WAVE data when we don't know the format yet");
		return nullptr;
	}
	
// todo : remember offset to data. keep parsing chunks. load data after having parsed all the chunks. this will make it possible to separate parsing the headers/chunks from the data, and create either a sound loader, or an audio stream implementation, using the same RIFF/WAVE header parsing code

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
