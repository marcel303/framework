#include "LgenFilterMean.h"
#include <stdlib.h>

namespace lgen
{
	void FilterMean::setMatrixSize(const int w, const int h)
	{
		matrixW = w;
		matrixH = h;
	}
	
    bool FilterMean::apply(const Heighfield & src, Heighfield & dst)
    {
    	int p;
    	
    	if (!src.getSizePowers(p, p))
    	{
    		return false;
    	}

    	int x1, y1, x2, y2;
    	
        getClippingRect(src, x1, y1, x2, y2);
        
        const int maskX = src.w - 1;
        const int maskY = src.h - 1;
        
        const int rx = (matrixW - 1) >> 1;
        const int ry = (matrixH - 1) >> 1;
        
        const int area = matrixW * matrixH;
        
        #define PIXEL(x, y) src.height[(x) & maskX][(y) & maskY]

        for (int i = x1; i <= x2; ++i)
        {
            for (int j = y1; j <= y2; ++j)
            {
                int sum = 0;
                
                for (int k = -rx; k <= rx; ++k)
                {
                    for (int l = -ry; l <= ry; ++l)
                    {
                        sum += PIXEL(i + k, j + l);
    				}
    			}
    			
                dst.height[i][j] = sum / area;
            }
        }

        return true;
    }

    bool FilterMean::setOption(const std::string & name, const char * value)
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
            return Filter::setOption(name, value);;
    	}

       return true;
    }
}
