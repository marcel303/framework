#include "LgenFilterMedian.h"
#include <stdlib.h>

namespace lgen
{
	bool FilterMedian::setMatrixSize(const int w, const int h)
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
	
    static int cb_sort(const void * e1, const void * e2)
    {
        const int v1 = *((const float*)e1);
        const int v2 = *((const float*)e2);

        return v1 - v2;
    }

    bool FilterMedian::apply(const Heightfield & src, Heightfield & dst)
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

        const int area = matrixW * matrixH;
        
        float * values = new float[area];
        
        const float * median = &values[(area - 1) >> 1];

        for (int i = x1; i <= x2; ++i)
        {
            for (int j = y1; j <= y2; ++j)
            {
                float * ptr = values;
                
                for (int k = -rx; k <= rx; ++k)
                {
                    for (int l = -ry; l <= ry; ++l)
                    {
                        *ptr++ = getHeight(src, i + k, j + l);
    				}
    			}
    			
                qsort(values, area, sizeof(float), cb_sort);
                
                dst.height[i][j] = *median;
            }
        }
        
        delete [] values;

        return true;
    }

    bool FilterMedian::setOption(const std::string & name, const char * value)
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
	
    bool filterMedian(const Heightfield & src, Heightfield & dst, const int matrixSx, const int matrixSy)
	{
		FilterMedian filter;
		return
			filter.setMatrixSize(matrixSx, matrixSy) &&
			filter.apply(src, dst);
	}
};
