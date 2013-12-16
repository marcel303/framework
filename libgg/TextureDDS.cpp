#include "TextureDDS.h"

TextureDDS::TextureDDS(TextureDDSType type, int sx, int sy, const char* fourCC, uint8_t* bytes, bool ownBytes, int byteCount)
{
	mType = type;
	mSx = sx;
	mSy = sy;
	mFourCC = fourCC;
	mBytes = bytes;
	mOwnBytes = ownBytes;
	mByteCount = byteCount;
}

TextureDDS::~TextureDDS()
{
	if (mOwnBytes)
	{
		delete[] mBytes;
		mBytes = 0;
	}
}
