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

#include "ogg/bitwise.c"
#include "ogg/framing.c"

#include "vorbis/window.h"

#include "vorbis/bitrate.c"

#define ilog2 block_ilog2
#include "vorbis/block.c"
#undef ilog2

#define bitreverse codebook_bitreverse
#include "vorbis/codebook.c"
#undef bitreverse

#include "vorbis/envelope.c"
#include "vorbis/floor0.c"

#define ilog floor1_ilog
#define ilog2 floor1_ilog2
#define FLOOR1_fromdB_LOOKUP floor1_FLOOR1_fromdB_LOOKUP
#include "vorbis/floor1.c"
#undef FLOOR1_fromdB_LOOKUP
#undef ilog
#undef ilog2

#define ilog2 info_ilog2
#include "vorbis/info.c"
#undef ilog2

#include "vorbis/lpc.c"
#include "vorbis/lsp.c"

#define ilog mapping0_ilog
#include "vorbis/mapping0.c"
#undef ilog

#include "vorbis/mdct.c"

#define FLOOR1_fromdB_LOOKUP psy_FLOOR1_fromdB_LOOKUP
#include "vorbis/psy.c"
#undef FLOOR1_fromdB_LOOKUP

#include "vorbis/registry.c"

#define ilog res0_ilog
#include "vorbis/res0.c"
#undef ilog

#define bitreverse sharedbook_bitreverse
#include "vorbis/sharedbook.c"
#undef bitreverse

#include "vorbis/smallft.c"
#include "vorbis/synthesis.c"
#include "vorbis/vorbisfile.c"
#include "vorbis/window.c"
