#include <stdlib.h>
#include "LgenOs.h"

namespace lgen
{
	bool LgenOs::generate()
	{
		// Current implementation expects w and h to be the same.
	        
		if (w != h)
		{
			return false;
		}
	                
		// Check if w and h are powers of 2.
		
		int p;
		
		if (!getSizePowers(p, p))
		{
			return false;
		}
	                
		#define RAND(x, y) ((x) + (rand() & (y)) - ((y) >> 1))
	        
		int rowOffset = 0;  // Start at zero for first row.
		const int mask = w - 1;
	    
		for (int squareSize = w; squareSize > 1; squareSize >>= 1)
		{
			const int randomRange = squareSize;

			for (int x1 = rowOffset; x1 < w; x1 += squareSize) 
			{
				for (int y1 = rowOffset; y1 < w; y1 += squareSize)
				{
					// Get the four corner points.

					int x2 = (x1 + squareSize) & mask;
					int y2 = (y1 + squareSize) & mask;

					const int i1 = height[x1][y1];
					const int i2 = height[x2][y1];
					const int i3 = height[x1][y2];
					const int i4 = height[x2][y2];

					// Obtain new points by averaging the corner points.

					int p1 = ((i1 * 9) + (i2 * 3) + (i3 * 3) + (i4)) >> 4;
					int p2 = ((i1 * 3) + (i2 * 9) + (i3) + (i4 * 3)) >> 4;
					int p3 = ((i1 * 3) + (i2) + (i3 * 9) + (i4 * 3)) >> 4;
					int p4 = ((i1) + (i2 * 3) + (i3 * 3) + (i4 * 9)) >> 4;

					// Add a random offset to each new point.

					p1 = RAND(p1, randomRange);
					p2 = RAND(p2, randomRange);
					p3 = RAND(p3, randomRange);
					p4 = RAND(p4, randomRange);

					// Write out the generated points.

					const int x3 = (x1 + (squareSize >> 2)) & mask;
					const int y3 = (y1 + (squareSize >> 2)) & mask;
					x2 = (x3 + (squareSize >> 1)) & mask;
					y2 = (y3 + (squareSize >> 1)) & mask;

					height[x3][y3] = p1;
					height[x2][y3] = p2;
					height[x3][y2] = p3;
					height[x2][y2] = p4;
				}
			}

			rowOffset = squareSize >> 2;  // Set offset for next row.
		}

		return true;
	}
}
