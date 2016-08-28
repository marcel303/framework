#pragma once

#include "libklodder_forward.h"

class ImageResampling
{
public:
	static void Blit_Transformed(MacImage & src, MacImage & dst, const BlitTransform & transform);
};
