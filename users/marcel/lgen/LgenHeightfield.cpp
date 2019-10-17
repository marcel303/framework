#include "LgenFilter.h"
#include "LgenHeightfield.h"

namespace lgen
{
	Heightfield::~Heightfield()
	{
		setSize(0, 0);
	}

	bool Heightfield::setSize(int a_w, int a_h)
	{
		// Don't bother if already this size.
		
		if (w == a_w && h == a_h)
		{
			return w > 0 && h > 0;
		}
		
		// Free memory.
	   	
		if (height)
		{
			for (int i = 0; i < w; ++i)
			{
				delete[] height[i];
			}
			
			delete[] height;
			
			height = nullptr;
			w = h = 0;
		}
		
		// Should we allocate new memory?

		if (a_w <= 0 || a_h <= 0)
		{
			return false;
		}

		// Allocate.
		
		height = new int*[a_w];
		
		for (int i = 0; i < a_w; ++i)
		{
			height[i] = new int[a_h];
		}

		w = a_w;
		h = a_h;

		return true;
	}

	void Heightfield::clear()
	{
		for (int i = 0; i < w; ++i)
		{
			for (int j = 0; j < h; ++j)
			{
				height[i][j] = 0;
			}
		}
	}

	void Heightfield::clamp(int min, int max)
	{
		// Swap if min > max.
		
		if (min > max)
		{
			int tmp = min;
			min = max;
			max = tmp;
		}

		for (int i = 0; i < w; ++i)
		{
			int * tmp = height[i];
			
			for (int j = 0; j < h; ++j)
			{
				if (*tmp < min)
				{
					*tmp = min;
				}
				else if (*tmp > max)
				{
					*tmp = max;
				}
				
				tmp++;
			}
		}
	}

	void Heightfield::rerange(int a_min, int a_max)
	{
		if (height == nullptr)
		{
			return;
		}

		// Swap if min > max.
		
		if (a_min > a_max) 
		{
			int tmp = a_min;
			a_min = a_max;
			a_max = tmp;
		}

		// Find min / max.
		
		int min = height[0][0];
		int max = height[0][0];

		for (int i = 0; i < w; ++i)
		{
			int * tmp = height[i];
			
			for (int j = 0; j < h; ++j)
			{
				if (*tmp < min)
				{
					min = *tmp;
				}
				else if (*tmp > max)
				{
					max = *tmp;
				}
				
				tmp++;
			}
		}

		// Get scaling factors.
		
		int s1 = max - min;
		
		if (!s1)
		{
			s1 = 1;
		}
		
		int s2 = a_max - a_min;

		// Translate and scale.
		
		for (int i = 0; i < w; ++i)
		{
			int * tmp = height[i];
			
			for (int j = 0; j < h; ++j)
			{
				const int value = *tmp;
				
				*tmp++ = a_min + (value - min) * s2 / s1;
			}
		}
	}

	void Heightfield::copyTo(Heightfield & dst) const
	{
		if (&dst == this)
			return;
			
	    dst.setSize(w, h);
	    
	    for (int i = 0; i < w; ++i)
	    {
	        int * tmp1 = height[i];
	        int * tmp2 = dst.height[i];
	        
	        for (int j = 0; j < h; ++j)
	        {
	            *tmp2++ = *tmp1++;
			}
	    }
	}

	bool Heightfield::getSizePowers(int & pw, int & ph) const
	{
		int t_w = w;
		int t_h = h;
		int t_pw = -1;
		int t_ph = -1;
		
		// Get powers.

		while (t_w > 0)
		{
			t_w >>= 1;
			++t_pw;
		}

		while (t_h > 0)
		{
			t_h >>= 1;
			++t_ph;
		}
		
		// Check if the values are correct (eg not a power of 2).

		if ((1 << t_pw) != w || (1 << t_ph) != h)
		{
			return false;
		}

		pw = t_pw;
		ph = t_ph;

		return true;
	}
	
	//
	
	bool DoubleBufferedHeightfield::setSize(int w, int h)
	{
		return
			heightfield[0].setSize(w, h) &&
			heightfield[1].setSize(w, h);
	}
	
	void DoubleBufferedHeightfield::clear()
	{
		return heightfield[currentIndex].clear();
	}

	void DoubleBufferedHeightfield::clamp(int min, int max)
	{
		return heightfield[currentIndex].clamp(min, max);
	}
	
	void DoubleBufferedHeightfield::rerange(int min, int max)
	{
		return heightfield[currentIndex].rerange(min, max);
	}
	
	void DoubleBufferedHeightfield::copyTo(Heightfield & dst) const
	{
		return heightfield[currentIndex].copyTo(dst);
	}
	
	bool DoubleBufferedHeightfield::getSizePowers(int & pw, int & ph) const
	{
		return heightfield[currentIndex].getSizePowers(pw, ph);
	}
	
	void DoubleBufferedHeightfield::swapBuffers()
	{
		currentIndex = 1 - currentIndex;
	}
	
	bool DoubleBufferedHeightfield::applyFilter(Filter & filter)
	{
		const bool result = filter.apply(heightfield[currentIndex], heightfield[1 - currentIndex]);
		
		swapBuffers();
		
		return result;
	}
}
