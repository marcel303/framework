#include "LgenFilter.h"
#include <algorithm>
#include <stdlib.h>

namespace lgen
{
	static inline int Mid(int min, int v, int max)
	{
		return (v < min ? min : v > max ? max : v);
	}

	Filter::~Filter()
	{
	}

	bool Filter::setOption(const std::string & name, const char * value)
	{
		if (name == "clip.x1")
		{
	    	clipX1 = atoi(value);
	    	return true;
		}
		else if (name == "clip.y1")
		{
	    	clipY1 = atoi(value);
	    	return true;
		}
		else if (name == "clip.x2")
		{
	    	clipX2 = atoi(value);
	    	return true;
		}
		else if (name == "clip.y2")
		{
	    	clipY2 = atoi(value);
	    	return true;
		}
		else
		{
	    	return false;
		}
	}

	float Filter::getHeight(const Heightfield & lgen, int x, int y) const
	{
		if (x >= 0 &&
			y >= 0 &&
			x < lgen.w &&
			y < lgen.h)
		{
			return lgen.height[x][y];
		}
			
		switch (borderMode)
		{		
		case bmClamp:
			{
				return lgen.height[Mid(0, x, lgen.w - 1)][Mid(0, y, lgen.h - 1)];
			}
			break;
			
		case bmWrap:
			{
				return lgen.height[x % lgen.w][y % lgen.h];
			}
			break;
			
		case bmMirror:
			{
				int tx = x % (lgen.w * 2 - 2);
				int ty = y % (lgen.h * 2 - 2);
				
				if (tx >= lgen.w)
				{
					tx = lgen.w * 2 - 1 - tx;
				}
				if (ty >= lgen.h)
				{
					ty = lgen.h * 2 - 1 - ty;
				}
			}
			break;
		}
		
		return 0;
	}

	void Filter::getClippingRect(const Heightfield & heightfield, int & x1, int & y1, int & x2, int & y2) const
	{
		// Transform clipping rectangle into pixel space.
	    
		x1 = clipX1;
		y1 = clipY1;
		x2 = clipX2;
		y2 = clipY2;
	    
		// Default is an 'infinitely' large square. Completely overlapping the heightmap anyways.
	    
		if (x2 < 0)
		{
			x2 = 1000000;
		}
		if (y2 < 0)
		{
			y2 = 1000000;
		}

		// Clip against sides of lgen.

	    #define CLIP(v, d) \
	    	if (v < 0) \
	    	{ \
	        	v = 0; \
			} \
			else if (v >= heightfield.d) \
			{ \
	        	v = heightfield.d - 1; \
			}

		CLIP(x1, w);
	    CLIP(y1, h);
	    CLIP(x2, w);
	    CLIP(y2, h);

	    // Make sure x1 <= x2 and y1 <= y2.
	    
		if (x1 > x2)
		{
	    	std::swap(x1, x2);
		}
		if (y1 > y2)
		{
	    	std::swap(y1, y2);
		}
	}
	
	void Filter::setClippingRect(int x1, int y1, int x2, int y2)
	{
		clipX1 = x1;
		clipY1 = y1;
		clipX2 = x2;
		clipY2 = y2;
	}
}
