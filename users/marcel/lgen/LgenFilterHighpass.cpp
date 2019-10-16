#include "LgenFilterHighpass.h"
#include <stdlib.h>
#include <string.h>

namespace lgen
{
	bool FilterHighpass::apply(const Heighfield * src, Heighfield * dst)
	{
		int p;
		
		if (!src->getSizePowers(p, p))
		{
			return false;
		}

		int x1, y1, x2, y2;
		
	 	getClippingRect(src, x1, y1, x2, y2);
	    
	    const int maskX = src->w - 1;
	    const int maskY = src->h - 1;

	    #define PIXEL(x, y) src->height[(x) & maskX][(y) & maskY]

	    // Highpass filter with 1.0 radius.. never really gives good results ;-).
	    
	    for (int i = x1; i <= x2; ++i)
	    {
	        for (int j = y1; j <= y2; ++j)
	        {
	            dst->height[i][j] =
					PIXEL(i + 0, j + 0) * 5 -
					PIXEL(i - 1, j + 0) -
					PIXEL(i + 1, j + 0) -
					PIXEL(i + 0, j - 1) -
					PIXEL(i + 0, j + 1);
	        }
	    }

	    return true;
	}

	bool FilterHighpass::setOption(const std::string & name, char* value)
	{
	    return Filter::setOption(name, value);
	}
}
