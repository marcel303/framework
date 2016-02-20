#pragma once

struct AVCodecContext;

namespace MP
{
	namespace Util
	{
		void SetDefaultCodecContextOptions(AVCodecContext* codecContext);
	}
};
