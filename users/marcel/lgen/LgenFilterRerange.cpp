#include "LgenFilterRerange.h"
#include <stdlib.h>

namespace lgen
{
	bool FilterRerange::setMinMax(const int in_min, const int in_max)
	{
		min = in_min;
		max = in_max;
		
		return true;
	}
	
    bool FilterRerange::apply(const Heightfield & src, Heightfield & dst)
    {
		int x1, y1, x2, y2;
		
        getClippingRect(src, x1, y1, x2, y2);
		
		// Swap if min > max.
		
		int a_min = min;
		int a_max = max;
		
		if (a_min > a_max)
		{
			int tmp = a_min;
			a_min = a_max;
			a_max = tmp;
		}

		// Find min / max.
		
		int min = dst.height[0][0];
		int max = dst.height[0][0];

		for (int i = x1; i <= x2; ++i)
		{
			int * tmp = dst.height[i] + y1;
			
			for (int j = y1; j <= y2; ++j)
			{
				if (*tmp < min)
				{
					min = *tmp;
				}
				else if (*tmp > max)
				{
					max = *tmp;
				}
				
				tmp++;
			}
		}

		// Get scaling factors.
		
		int s1 = max - min;
		
		if (!s1)
		{
			s1 = 1;
		}
		
		int s2 = a_max - a_min;

		// Translate and scale.
		
		for (int i = x1; i <= x2; ++i)
		{
			int * tmp = dst.height[i] + y1;
			
			for (int j = y1; j <= y2; ++j)
			{
				const int value = *tmp;
				
				*tmp++ = a_min + (value - min) * s2 / s1;
			}
		}

        return true;
    }

    bool FilterRerange::setOption(const std::string & name, const char * value)
    {
        if (name == "min")
        {
            min = atoi(value);
			
            return true;
        }
    	else if (name == "max")
        {
            max = atoi(value);
			
            return true;
        }
    	else
        {
            return Filter::setOption(name, value);
    	}
    }
	
    //
	
    bool filterRerange(const Heightfield & src, Heightfield & dst, const int min, const int max)
	{
		FilterRerange filter;
		return
			filter.setMinMax(min, max) &&
			filter.apply(src, dst);
	}
}
