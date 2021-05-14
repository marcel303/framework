#include "LgenFilterQuantize.h"
#include <math.h>
#include <stdlib.h>

namespace lgen
{
	bool FilterQuantize::setNumLevels(const int in_numLevels)
	{
		if (in_numLevels < 2)
		{
			return false;
		}
		else
		{
			numLevels = in_numLevels;

			return true;
		}
	}
	
    bool FilterQuantize::apply(const Heightfield & src, Heightfield & dst)
    {
        int x1, y1, x2, y2;
        
        getClippingRect(src, x1, y1, x2, y2);
        
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
        
        float s2 = max - min;

        // Translate and scale.
        
        for (int i = x1; i <= x2; ++i)
        {
            const float * src_itr = src.height[i] + y1;
                  float * dst_itr = dst.height[i] + y1;
            
            for (int j = y1; j <= y2; ++j)
            {
                const float value = *src_itr++;
				
                const int level = (value - min) * (numLevels - 1e-3f) / s1;
                
                *dst_itr++ = min + level * s2 / (numLevels - 1);
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
