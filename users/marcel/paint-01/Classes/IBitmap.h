#pragma once

#include "Col.h"
#include "Types.h"

GG_INTERFACE_BEGIN(IBitmap)
	GG_INTERFACE_METHOD(UInt8*, GetPixel, (int x, int y));
	GG_INTERFACE_METHOD(IBitmap*, SubBitmap, (int x, int y, int sx, int sy));
	GG_INTERFACE_METHOD(void, DrawLine, (int x1, int x2, int y, const Paint::Col* c));
GG_INTERFACE_END()
