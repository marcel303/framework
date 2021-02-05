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

#include "WaveLoader.h"

#include "../internal_filereader.h"

#include "Log.h"

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
		else if (checkId(id, "LIST") || checkId(id, "FLLR") || checkId(id, "JUNK") || checkId(id, "bext") || checkId(id, "fact") || checkId(id, "attn") || checkId(id, "_PMX"))
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

bool WaveHeadersReader::read(FILE * file)
{
	FileReader r(file);
	
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
			
			//if (fmtCompressionType != 1)
				ok &= r.read(fmtExtraLength);
			//else
			//	fmtExtraLength = 0;
			
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
