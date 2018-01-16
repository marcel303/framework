/*
	Copyright (C) 2017 Marcel Smit
	marcel303@gmail.com
	https://www.facebook.com/marcel.smit981

	Permission is hereby granted, free of charge, to any person
	obtaining a copy of this software and associated documentation
	files (the "Software"), to deal in the Software without
	restriction, including without limitation the rights to use,
	copy, modify, merge, publish, distribute, sublicense, and/or
	sell copies of the Software, and to permit persons to whom the
	Software is furnished to do so, subject to the following
	conditions:

	The above copyright notice and this permission notice shall be
	included in all copies or substantial portions of the Software.

	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
	EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
	OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
	NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
	HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
	WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
	FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
	OTHER DEALINGS IN THE SOFTWARE.
*/

#include "MPUtil.h"

extern "C"
{
	#include <libavcodec/avcodec.h>
	#include <libavformat/avformat.h>
}

namespace MP
{
	namespace Util
	{
		static bool s_avcodecInitialized = false;

		void InitializeLibAvcodec()
		{
			if (s_avcodecInitialized == false)
			{
				s_avcodecInitialized = true;

				// Initialize libavcodec by registering all codecs.
				av_register_all();
			#if !defined(DEBUG) || 1
				av_log_set_level(AV_LOG_QUIET);
			#endif
			}
		}

		void SetDefaultCodecContextOptions(AVCodecContext * codecContext)
		{
			// Define options.
			bool lowres                = false; // Decode at full resolution.
			bool fast                  = false; // Allow non-compliant speed-up yes/no.
			int debug                  = 0; // No debugging.
			int debug_mv               = 0; // No 'mv' debugging.
			//int error_resilience       = FF_ER_CAREFUL;
			int error_concealment      = 3; // Conceal all.
			int workaround_bugs        = FF_BUG_AUTODETECT; // Auto-detect bugs.
			int idct                   = FF_IDCT_AUTO;
			AVDiscard skip_frame       = AVDISCARD_DEFAULT;
			AVDiscard skip_idct        = AVDISCARD_DEFAULT;
			AVDiscard skip_loop_filter = AVDISCARD_DEFAULT;

			// Fill codec context.
			codecContext->debug_mv          = debug_mv;
			codecContext->debug             = debug;
			codecContext->workaround_bugs   = workaround_bugs;
			codecContext->lowres            = lowres;
			codecContext->idct_algo         = idct;
			codecContext->skip_frame        = skip_frame;
			codecContext->skip_idct         = skip_idct;
			codecContext->skip_loop_filter  = skip_loop_filter;
			//codecContext->error_resilience  = error_resilience;
			codecContext->error_concealment = error_concealment;

			if (lowres)
				codecContext->flags |= CODEC_FLAG_EMU_EDGE;

			if (fast)
				codecContext->flags2 |= CODEC_FLAG2_FAST;
		}
	};
};
