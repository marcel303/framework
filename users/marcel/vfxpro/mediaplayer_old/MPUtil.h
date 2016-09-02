#pragma once

#include <ffmpeg_old/avcodec.h>

namespace MP
{
	namespace Util
	{
		void InitializeLibAvcodec();

		void SetDefaultCodecContextOptions(AVCodecContext* codecContext);
	}
};
