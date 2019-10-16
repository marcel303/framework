#include <stdlib.h>
#include <string.h>
#include "LgenFilterMaximum.h"

namespace Lgen
{
	
FilterMaximum::FilterMaximum() : Filter()
{

    matrixW = 3;
	matrixH = 3;

}

FilterMaximum::~FilterMaximum()
{

}

bool FilterMaximum::Apply(Lgen* src, Lgen* dst)
{

	int p;
	
	if (!src->GetSizePowers(p, p))
	{
		return false;
	}

	int x1, y1, x2, y2;
	
    GetClippingRect(src, x1, y1, x2, y2);
    
    int maskX = src->w - 1;
    int maskY = src->h - 1;
    
    int rx = (matrixW - 1) / 2;
    int ry = (matrixH - 1) / 2;

    #define PIXEL(x, y) src->height[(x) & maskX][(y) & maskY]

    for (int i = x1; i <= x2; ++i)
    {
        for (int j = y1; j <= y2; ++j)
        {
			
            int max = PIXEL(i, j);
            
            for (int k = -rx; k <= rx; ++k)
            {
                for (int l = -ry; l <= ry; ++l)
                {
                    if (PIXEL(i + k, j + l) > max)
                    {
                        max = PIXEL(i + k, j + l);
					}
				}
			}
				
            dst->height[i][j] = max;
            
        }
    }

    return true;

}

bool FilterMaximum::SetOption(std::string name, char* value)
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
        return Filter::SetOption(name, value);
	}

   return true;

}

};
