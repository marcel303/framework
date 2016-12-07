#pragma once

#include <ffmpeg/avcodec.h>

namespace MP
{
	namespace Util
	{
		void InitializeLibAvcodec();

		void SetDefaultCodecContextOptions(AVCodecContext* codecContext);
	}
};
