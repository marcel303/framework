#include "LgenFilterMapMaximum.h"
#include <stdlib.h>

namespace lgen
{
	bool FilterMapMaximum::setMax(const int in_max)
	{
		max = in_max;
		
		return true;
	}
	
    bool FilterMapMaximum::apply(const Heightfield & src, Heightfield & dst)
    {
		int x1, y1, x2, y2;
		
        getClippingRect(src, x1, y1, x2, y2);
		
		const int a_min = 0;
		const int a_max = max;

		// Find min / max.
		
		const int min = 0;
		int max = dst.height[0][0];

		for (int i = x1; i <= x2; ++i)
		{
			int * tmp = dst.height[i] + y1;
			
			for (int j = y1; j <= y2; ++j)
			{
				if (*tmp > max)
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

    bool FilterMapMaximum::setOption(const std::string & name, const char * value)
    {
        if (name == "max")
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
	
    bool filterMapMaximum(const Heightfield & src, Heightfield & dst, const int max)
	{
		FilterMapMaximum filter;
		return
			filter.setMax(max) &&
			filter.apply(src, dst);
	}
}
