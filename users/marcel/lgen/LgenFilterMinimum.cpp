#include "LgenFilterMinimum.h"
#include <stdlib.h>

namespace lgen
{
	bool FilterMinimum::setMatrixSize(const int w, const int h)
	{
		if (w & 0x1 && w >= 3 &&
			h & 0x1 && h >= 3)
		{
			matrixW = w;
			matrixH = h;
			
			return true;
		}
		else
		{
			return false;
		}
	}
	
    bool FilterMinimum::apply(const Heightfield & src, Heightfield & dst)
    {
    	int p;
    	
    	if (!src.getSizePowers(p, p))
    	{
    		return false;
    	}

    	int x1, y1, x2, y2;
    	
        getClippingRect(src, x1, y1, x2, y2);
        
        const int rx = (matrixW - 1) >> 1;
        const int ry = (matrixH - 1) >> 1;

        for (int i = x1; i <= x2; ++i)
        {
            for (int j = y1; j <= y2; ++j)
            {
                float min = src.height[i][j];
                
                for (int k = -rx; k <= rx; ++k)
                {
                    for (int l = -ry; l <= ry; ++l)
                    {
						float value = getHeight(src, i + k, j + l);
						
                        if (value < min)
                        {
                            min = value;
    					}
    				}
    			}
    			
                dst.height[i][j] = min;
            }
        }

        return true;
    }

    bool FilterMinimum::setOption(const std::string & name, const char * value)
    {
        if (name == "matrix.size")
        {
            int s = atoi(value);
            
            if (s & 0x1 && s >= 3)
            {
                matrixW = s;
                matrixH = s;
				
                return true;
            }
    		else
    		{
                return false;
    		}
        }
    	else if (name == "matrix.width")
        {
            int w = atoi(value);
            
            if (w & 0x1 && w >= 3)
            {
                matrixW = w;
				
                return true;
    		}
            else
            {
                return false;
    		}
        }
    	else if (name == "matrix.height")
        {
            int h = atoi(value);
            
            if (h & 0x1 && h >= 3)
            {
                matrixH = h;
				
                return true;
    		}
            else
            {
                return false;
    		}
        }
    	else
        {
            return Filter::setOption(name, value);
    	}
    }
	
    //
	
    bool filterMinimum(const Heightfield & src, Heightfield & dst, const int matrixSx, const int matrixSy)
	{
		FilterMinimum filter;
		return
			filter.setMatrixSize(matrixSx, matrixSy) &&
			filter.apply(src, dst);
	}
}
