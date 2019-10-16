#include <stdlib.h>
#include <string.h>
#include "LgenFilterMinimum.h"

namespace Lgen
{
	
static int cb_sort(const void* e1, const void* e2)
{
    const int v1 = *((const int*)e1);
    const int v2 = *((const int*)e2);
    return v1 - v2;
}

FilterMedian::FilterMedian() : Filter()
{

    matrixW = 3;
	matrixH = 3;

}

FilterMedian :: ~FilterMedian()
{

}

bool FilterMedian::Apply(Lgen* src, Lgen* dst)
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
    
    int rx = (matrixW - 1) >> 1;
    int ry = (matrixH - 1) >> 1;

    const int area = matrixW * matrixH;
    
    int* values = new int[area];
    
    const int* median = &values[(area - 1) >> 1];

    #define PIXEL(x, y) src->height[(x) & maskX][(y) & maskY]

    for (int i = x1; i <= x2; ++i)
    {
        for (int j = y1; j <= y2; ++j)
        {
			
            int* ptr = values;
            
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
    
    delete[] values;

    return true;

}

bool FilterMedian::SetOption(std::string name, char* value)
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
