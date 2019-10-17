#include "LgenFilterConvolution.h"

namespace lgen
{
	FilterConvolution::~FilterConvolution()
	{
		setExtents(-1, -1);
	}
	
	bool FilterConvolution::apply(const Heightfield & src, Heightfield & dst)
	{
		int x1, y1, x2, y2;
		
		getClippingRect(src, x1, y1, x2, y2);
	    
		for (int x = x1; x <= x2; ++x)
		{
			for (int y = y1; y <= y2; ++y)
			{
				float v = 0.0;
				
				for (int mx = -ex; mx <= ex; ++mx)
				{
					for (int my = -ey; my <= ey; ++my)
					{
						v += getHeight(src, x + mx, y + my) * matrix[mx + ex][my + ey];
					}
				}
				
				dst.height[x][y] = (int)v;
			}
		}
		
		return true;
	}

	bool FilterConvolution::setOption(const std::string & name, const char * value)
	{
	    return Filter::setOption(name, value);
	}

	void FilterConvolution::setExtents(int ex, int ey)
	{
	    if (matrix != nullptr)
	    {
	        for (int x = 0; x < this->ex * 2 + 1; ++x)
	            delete [] matrix[x];
	            
	        delete [] matrix;
	        
	        matrix = nullptr;
	        
	        this->ex = -1;
			this->ey = -1;
	    }
	    
	    if (ex < 0 || ey < 0)
	    {
	    	return;
		}
		
	    matrix = new float*[ex * 2 + 1];
	    
	    for (int x = 0; x < ex * 2 + 1; ++x)
	    {
	        matrix[x] = new float[ey * 2 + 1];
		}
		   
	    this->ex = ex;
	    this->ey = ey;

	    loadIdentity();
	}

	void FilterConvolution::loadIdentity()
	{
	    for (int x = 0; x < ex * 2 + 1; ++x)
	    {
	    	for (int y = 0; y < ey * 2 + 1; ++y)
	    	{
	            matrix[x][y] = 0.0f;
	    	}
		}
	    	
	    matrix[ex][ey] = 1.0f;
	}
}
