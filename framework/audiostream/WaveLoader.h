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

#pragma once

#include <stdint.h>
#include <stdio.h>

class FileReader;

// RIFF/WAVE loader constants

#define WAVE_FORMAT_PCM 1
#define WAVE_FORMAT_IEEE_FLOAT 3
#define WAVE_FORMAT_EXTENSIBLE -2

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
	
	bool read(FILE * file);
};
