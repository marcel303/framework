#include "LgenFilterRerange.h"
#include <stdlib.h>

namespace lgen
{
	bool FilterRerange::setMinMax(const float in_min, const float in_max)
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
		
		float a_min = min;
		float a_max = max;
		
		if (a_min > a_max)
		{
			float tmp = a_min;
			a_min = a_max;
			a_max = tmp;
		}

		// Find min / max.
		
		float min = src.height[x1][y1];
		float max = src.height[x1][y1];

		for (int i = x1; i <= x2; ++i)
		{
			const float * src_itr = src.height[i] + y1;
			
			for (int j = y1; j <= y2; ++j)
			{
				const float value = *src_itr++;
				
				if (value < min)
				{
					min = value;
				}
				else if (value > max)
				{
					max = value;
				}
			}
		}

		// Get scaling factors.
		
		float s1 = max - min;
		
		if (s1 == 0.f)
		{
			s1 = 1.f;
		}
		
		float s2 = a_max - a_min;

		// Translate and scale.
		
		for (int i = x1; i <= x2; ++i)
		{
			const float * src_itr = src.height[i] + y1;
			      float * dst_itr = dst.height[i] + y1;
			
			for (int j = y1; j <= y2; ++j)
			{
				const float value = *src_itr++;
				
				*dst_itr++ = a_min + (value - min) * s2 / s1;
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
	
    bool filterRerange(const Heightfield & src, Heightfield & dst, const float min, const float max)
	{
		FilterRerange filter;
		return
			filter.setMinMax(min, max) &&
			filter.apply(src, dst);
	}
}
