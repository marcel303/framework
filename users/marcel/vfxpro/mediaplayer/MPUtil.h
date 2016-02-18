#pragma once

#include <libavcodec/avcodec.h>

namespace MP
{
	namespace Util
	{
		void SetDefaultCodecContextOptions(AVCodecContext* codecContext);
	}
};
