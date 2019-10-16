#include <string.h>
#include "LgenFilterConvolution.h"

namespace Lgen
{
	
FilterConvolution::FilterConvolution() : Filter()
{

	ex = ey = -1;
	
	matrix = 0;

}

FilterConvolution::~FilterConvolution()
{
	
}

bool FilterConvolution::Apply(Lgen* src, Lgen* dst)
{

	int x1, y1, x2, y2;
	
	GetClippingRect(src, x1, y1, x2, y2);
    
	for (int x = x1; x <= x2; ++x)
	{
		for (int y = y1; y <= y2; ++y)
		{
			
			float v = 0.0;
			
			for (int mx = -ex; mx <= ex; ++mx)
			{
				for (int my = -ey; my <= ey; ++my)
				{
					v += GetHeight(src, x + mx, y + my) * matrix[mx + ex][my + ey];
				}
			}
			
			dst->height[x][y] = (int)v;
			
		}
	}
	
	return true;

}

bool FilterConvolution::SetOption(std::string name, char* value)
{

    if (name == "file.load")
    {
    
    	// Read convolution matrix from file.
    
    }
	else if (name == "file.save")
	{
		
		// Write convolution matrix to file.
		
	}
    
    return true;

}

void FilterConvolution::SetExtents(int ex, int ey)
{

    if (matrix)
    {
		
        for (int x = 0; x < this->ex * 2 + 1; ++x)
            delete[] matrix[x];
            
        delete[] matrix;
        
        matrix = 0;
        
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

    LoadIdentity();

}

void FilterConvolution::LoadIdentity()
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

};

