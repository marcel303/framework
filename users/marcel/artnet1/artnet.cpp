/*
	Copyright (C) 2017 Marcel Smit
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

#include "artnet.h"
#include <string.h>

static const char * kArtnetID = "Art-Net";

enum ArtnetOpcode
{
	kArtnetOpcode_ArtDMX = 0x5000
};

enum ArtnetVersion
{
	kArtnetVersion_14 = 14
};

static inline void writeByte(uint8_t *& dst, const uint8_t byte)
{
	*dst++ = byte;
}

static inline void writeBytes(uint8_t *& dst, const void * src, const int srcSize)
{
	memcpy(dst, src, srcSize);
	dst += srcSize;
}

static inline uint8_t lo16(const uint16_t value)
{
	return value & 0xff;
}

static inline uint8_t hi16(const uint16_t value)
{
	return (value >> 8) & 0xff;
}

bool ArtnetPacket::makeDMX512(
	const uint8_t sequence, const uint8_t physical, const uint16_t universe,
	const uint8_t * __restrict values, const int numValues)
{
	if (numValues > 512)
		return false;

	const uint16_t opcode = kArtnetOpcode_ArtDMX;
	const uint16_t version = kArtnetVersion_14;
	const uint16_t length = numValues;

	uint8_t * __restrict p = data;
	
	writeBytes(p, kArtnetID, sizeof(kArtnetID));
	writeByte(p, lo16(opcode));
	writeByte(p, hi16(opcode));
	writeByte(p, hi16(version));
	writeByte(p, lo16(version));
	writeByte(p, sequence);
	writeByte(p, physical);
	writeByte(p, lo16(universe));
	writeByte(p, hi16(universe));
	writeByte(p, hi16(length));
	writeByte(p, lo16(length));
	writeBytes(p, values, numValues * sizeof(uint8_t));

	dataSize = p - data;

	return true;
}
