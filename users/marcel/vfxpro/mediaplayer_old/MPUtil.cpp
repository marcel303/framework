#include "MPUtil.h"

namespace MP
{
	namespace Util
	{
		void SetDefaultCodecContextOptions(AVCodecContext* codecContext)
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
