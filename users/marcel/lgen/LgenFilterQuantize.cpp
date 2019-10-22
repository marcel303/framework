#include "LgenFilterQuantize.h"
#include <math.h>
#include <stdlib.h>

namespace lgen
{
	bool FilterQuantize::setNumLevels(const int in_numLevels)
	{
		numLevels = in_numLevels;

        return true;
	}
	
    bool FilterQuantize::apply(const Heightfield & src, Heightfield & dst)
    {
        int x1, y1, x2, y2;
        
        getClippingRect(src, x1, y1, x2, y2);
        
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

        const int a_min = min;
        const int a_max = max;

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
				
                const int level = (value - min) * numLevels / (s2 + 1);
                
                *tmp++ = a_min + level * s2 / (numLevels - 1);
            }
        }

        return true;
    }

    bool FilterQuantize::setOption(const std::string & name, const char * value)
    {
    	if (name == "levels")
    	{
            int numLevels = atoi(value);
            return setNumLevels(numLevels);
        }
		else
		{
			return Filter::setOption(name, value);
		}
    }
	
	//
	
	bool filterQuantize(const Heightfield & src, Heightfield & dst, const int numLevels)
	{
		FilterQuantize filter;
		return
			filter.setNumLevels(numLevels) &&
			filter.apply(src, dst);
	}
}
