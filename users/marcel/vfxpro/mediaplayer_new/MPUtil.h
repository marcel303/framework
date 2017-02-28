#pragma once

#include "MPForward.h"

namespace MP
{
	namespace Util
	{
		void InitializeLibAvcodec();

		void SetDefaultCodecContextOptions(AVCodecContext * codecContext);
	}
};
