#pragma once

#include <libavcodec/avcodec.h>

namespace MP
{
	namespace Util
	{
		void InitializeLibAvcodec();

		void SetDefaultCodecContextOptions(AVCodecContext* codecContext);
	}
};
