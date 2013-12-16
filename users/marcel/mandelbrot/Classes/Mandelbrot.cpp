#include "MacImage.h"
#include "Mandelbrot.h"

#define MAX 63

inline int Iterate(float x, float y)
{
	// z = (x, y)
	// z^2 + c = x*x + y*y + 2*x*y
	
	register float zx = x;
	register float zy = y;
	
	for (int i = MAX; i; --i)
	{
		const float zx2 = zx * zx;
		const float zy2 = zy * zy;
		
		if (zx2 + zy2 >= 4.0f)
			return i;
		
		zy = 2.0f * zx*zy + y;
		zx = zx2 - zy2 + x;
	}
	
	return 0;
}

void MbRender(MacImage& image, RectF rect)
{
	const float dx_sx = rect.m_Size[0] / image.Sx_get();
	const float dy_sy = rect.m_Size[1] / image.Sy_get();
	
	const int scale = 255 / MAX;
	
	float yf = rect.m_Position[1];
	
	for (int y = 0; y < image.Sy_get(); ++y)
	{
		MacRgba* line = image.Line_get(y);
		
		float xf = rect.m_Position[0];
		
		for (int x = image.Sx_get(); x; --x)
		{
			const int c = Iterate(xf, yf) * scale;
			
			line->rgba[0] = c;
			line->rgba[1] = c >> 1;
			line->rgba[2] = c >> 2;
			line->rgba[3] = 255;
			
			++line;
			
			xf += dx_sx;
		}
		
		yf += dy_sy;
	}
}

