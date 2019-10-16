#include "LgenFilterMinimum.h"
#include <stdlib.h>
#include <string.h>

namespace lgen
{
    static int cb_sort(const void * e1, const void * e2)
    {
        const int v1 = *((const int*)e1);
        const int v2 = *((const int*)e2);

        return v1 - v2;
    }

    bool FilterMedian::apply(const Generator * src, Generator * dst)
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
        
        const int rx = (matrixW - 1) >> 1;
        const int ry = (matrixH - 1) >> 1;

        const int area = matrixW * matrixH;
        
        int * values = new int[area];
        
        const int * median = &values[(area - 1) >> 1];

        #define PIXEL(x, y) src->height[(x) & maskX][(y) & maskY]

        for (int i = x1; i <= x2; ++i)
        {
            for (int j = y1; j <= y2; ++j)
            {
                int * ptr = values;
                
                for (int k = -rx; k <= rx; ++k)
                {
                    for (int l = -ry; l <= ry; ++l)
                    {
                        *ptr++ = PIXEL(i + k, j + l);
    				}
    			}
    			
                qsort(values, area, sizeof(int), cb_sort);
                
                dst->height[i][j] = *median;
            }
        }
        
        delete [] values;

        return true;
    }

    bool FilterMedian::setOption(const std::string & name, char * value)
    {
        if (name == "matrix.size")
        {
            int s = atoi(value);
            
            if (s & 0x1 && s >= 3)
            {
                matrixW = s;
                matrixH = s;
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

        return true;
    }
};
