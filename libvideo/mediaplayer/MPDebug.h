/*
	Copyright (C) 2020 Marcel Smit
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

#ifndef __MPDEBUG_H__
#define __MPDEBUG_H__

#if !defined(DEBUG_MEDIAPLAYER)
	#if defined(DEBUG)
		#define DEBUG_MEDIAPLAYER 0
	#else
		#define DEBUG_MEDIAPLAYER 0 // do not alter
	#endif
#endif

#if DEBUG_MEDIAPLAYER
	#if !defined(DEBUG_MEDIAPLAYER_LOGGING)
		#define DEBUG_MEDIAPLAYER_LOGGING 0
	#endif
	#if !defined(DEBUG_MEDIAPLAYER_VIDEO_ALLOCS)
		#define DEBUG_MEDIAPLAYER_VIDEO_ALLOCS 1
	#endif
	#if !defined(DEBUG_MEDIAPLAYER_SIMULATE_HICKUPS)
		#define DEBUG_MEDIAPLAYER_SIMULATE_HICKUPS 1
	#endif
#else
	#if !defined(DEBUG_MEDIAPLAYER_LOGGING)
		#define DEBUG_MEDIAPLAYER_LOGGING 0 // do not alter
	#endif
	#if !defined(DEBUG_MEDIAPLAYER_VIDEO_ALLOCS)
		#define DEBUG_MEDIAPLAYER_VIDEO_ALLOCS 0 // do not alter
	#endif
	#if !defined(DEBUG_MEDIAPLAYER_SIMULATE_HICKUPS)
		#define DEBUG_MEDIAPLAYER_SIMULATE_HICKUPS 0 // do not alter
	#endif
#endif

namespace MP
{
	namespace Debug
	{
        void Print(const char * format, ...);
		
        void SimulateHickup();
	};
};

#endif
