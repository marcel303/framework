#pragma once

#include "FixedSizeString.h"
#include "Types.h"

enum TextureDDSType
{
	TextureDDSType_DXT1,
	TextureDDSType_DXT3,
	TextureDDSType_DXT5
};

class TextureDDS
{
public:
	TextureDDS(TextureDDSType type, int sx, int sy, const char* fourCC, uint8_t* bytes, bool ownBytes, int byteCount);
	~TextureDDS();

	TextureDDSType mType;
	int mSx;
	int mSy;
	FixedSizeString<4> mFourCC;
	uint8_t* mBytes;
	bool mOwnBytes;
	int mByteCount;
};
