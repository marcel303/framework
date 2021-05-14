#include "LgenFilterMapMaximum.h"
#include <stdlib.h>

namespace lgen
{
	bool FilterMapMaximum::setMax(const float in_max)
	{
		max = in_max;
		
		return true;
	}
	
    bool FilterMapMaximum::apply(const Heightfield & src, Heightfield & dst)
    {
		int x1, y1, x2, y2;
		
        getClippingRect(src, x1, y1, x2, y2);
		
		const float a_min = 0.f;
		const float a_max = max;

		// Find min / max.
		
		const float min = 0.f;
		float max = src.height[x1][y1];

		for (int i = x1; i <= x2; ++i)
		{
			float * tmp = src.height[i] + y1;
			
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
		
		float s1 = max - min;
		
		if (s1 == 0.f)
		{
			s1 = 1;
		}
		
		float s2 = a_max - a_min;

		// Translate and scale.
		
		for (int i = x1; i <= x2; ++i)
		{
			const float * src_itr = src.height[i] + y1;
			      float * dst_itr = dst.height[i] + y1;
			
			for (int j = y1; j <= y2; ++j)
			{
				const float value = *src_itr;
				
				*dst_itr++ = a_min + (value - min) * s2 / s1;
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
	
    bool filterMapMaximum(const Heightfield & src, Heightfield & dst, const float max)
	{
		FilterMapMaximum filter;
		return
			filter.setMax(max) &&
			filter.apply(src, dst);
	}
}
