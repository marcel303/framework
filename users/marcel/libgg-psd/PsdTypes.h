#pragma once

#include <string>
#include "libgg_forward.h"
#include "Types.h"

#define PsdBlendKey
	// 'norm'=normal (?)
	// 'levl'=levels
	// 'curv'=curves
	// 'brit'=brightness/contrast
	// 'blnc'=color balance
	// 'hue '=old hue/saturation, Photoshop 4.0 
	// 'hue2'=new hue/saturation, Photoshop 5.0 
	// 'selc'=selective color 
	// 'thrs'=threshold 
	// 'nvrt'=invert 
	// 'post'=posterize

enum PsdCompressionType
{
	PsdCompressionType_Raw = 0,
	PsdCompressionType_Rle = 1
};

enum PsdResourceId
{
	PsdResourceId_ResolutionInfo = 1005,
	PsdResourceId_ChannelNames = 1006,
	PsdResourceId_DisplayInfo = 1007
};

enum PsdResolutionUnit
{
	PsdResolutionUnit_Inch = 1,
	PsdResolutionUnit_Cm = 2,
	PsdResolutionUnit_Px = 3,
	PsdResolutionUnit_Pica = 4,
	PsdResolutionUnit_Columns = 5
};

enum PsdColorSpace
{
	PsdColorSpace_Rgb = 0,
	PsdColorSpace_Hsb = 1,
	PsdColorSpace_Cmyk = 2,
	PsdColorSpace_Lab = 7,
	PsdColorSpace_Gray = 8
};

enum PsdImageMode
{
	PsdImageMode_Bitmap = 0,
	PsdImageMode_Gray = 1,
	PsdImageMode_Indexed = 2,
	PsdImageMode_Rgb = 3,
	PsdImageMode_Cmyk = 4,
	PsdImageMode_Multichannel = 7,
	PsdImageMode_Duotone = 8,
	PsdImageMode_Lab = 9
};

//

static inline int16_t Swap16(int16_t v)
{
	int8_t* ptr = (int8_t*)&v;

	std::swap(ptr[0], ptr[1]);

	int16_t* r = (int16_t*)ptr;

	return *r;
}

static inline uint16_t SwapU16(uint16_t v)
{
	uint8_t v1 = (v >> 0) & 0xFF;
	uint8_t v2 = (v >> 8) & 0xFF;

	return (v1 << 8) | (v2 << 0);
}

static inline uint32_t SwapU32(uint32_t v)
{
	uint8_t v1 = (v >> 0) & 0xFF;
	uint8_t v2 = (v >> 8) & 0xFF;
	uint8_t v3 = (v >> 16) & 0xFF;
	uint8_t v4 = (v >> 24) & 0xFF;

	return (v1 << 24) | (v2 << 16) | (v3 << 8) | (v4 << 0);
}

class PsdPascalString
{
public:
	static std::string Read(Stream* stream, int padding);
	static void Write(Stream* stream, const std::string& text, int padding);
};
