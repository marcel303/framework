#ifndef __MPDEBUG_H__
#define __MPDEBUG_H__

#if defined(DEBUG)
	#define DEBUG_MEDIAPLAYER 0 // fixme : make config option
#else
	#define DEBUG_MEDIAPLAYER 0 // do not alter
#endif

namespace MP
{
	namespace Debug
	{
        void Print(const char * format, ...);
	};
};

#endif
