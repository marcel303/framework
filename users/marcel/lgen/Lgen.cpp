#include "Lgen.h"

namespace lgen
{
	Lgen::~Lgen()
	{
		setSize(0, 0);
	}

	bool Lgen::setSize(int a_w, int a_h)
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
			
			height = 0;
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

	void Lgen::clear()
	{
		for (int i = 0; i < w; ++i)
		{
			for (int j = 0; j < h; ++j)
			{
				height[i][j] = 0;
			}
		}
	}

	bool Lgen::generate()
	{
		return false;
	}

	void Lgen::clamp(int min, int max)
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

	void Lgen::rerange(int a_min, int a_max)
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

	void Lgen::copy(Lgen * dst) const
	{
	    dst->setSize(w, h);
	    
	    for (int i = 0; i < w; ++i)
	    {
	        int * tmp1 = height[i];
	        int * tmp2 = dst->height[i];
	        
	        for (int j = 0; j < h; ++j)
	        {
	            *tmp2++ = *tmp1++;
			}
	    }
	}

	bool Lgen::getSizePowers(int & pw, int & ph) const
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
}
