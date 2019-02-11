#pragma once

#include <vector>
#include "libgg_forward.h"
#include "Types.h"

namespace PsdCompression
{
	int RleRepeatCount(const uint8_t* bytes, int _x, int sx);
	int RleNoRepeatCount(const uint8_t* bytes, int _x, int sx);
	void RleCompress(uint8_t* bytes, int sx, int sy, Stream* stream, std::vector<int>& rleHeader);
}
