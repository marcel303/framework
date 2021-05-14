#include "LgenFilter.h"
#include "LgenFilterMapMaximum.h"
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
		
		height = new float*[a_w];
		
		for (int i = 0; i < a_w; ++i)
		{
			height[i] = new float[a_h];
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
				height[i][j] = 0.f;
			}
		}
	}

	void Heightfield::clamp(float min, float max)
	{
		// Swap if min > max.
		
		if (min > max)
		{
			float tmp = min;
			min = max;
			max = tmp;
		}

		for (int i = 0; i < w; ++i)
		{
			float * tmp = height[i];
			
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

	void Heightfield::rerange(float a_min, float a_max)
	{
		if (height == nullptr)
		{
			return;
		}

		// Swap if min > max.
		
		if (a_min > a_max) 
		{
			float tmp = a_min;
			a_min = a_max;
			a_max = tmp;
		}

		// Find min / max.
		
		float min = height[0][0];
		float max = height[0][0];

		for (int i = 0; i < w; ++i)
		{
			float * tmp = height[i];
			
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
		
		float s1 = max - min;
		
		if (s1 == 0.f)
		{
			s1 = 1.f;
		}
		
		float s2 = a_max - a_min;

		// Translate and scale.
		
		for (int i = 0; i < w; ++i)
		{
			float * tmp = height[i];
			
			for (int j = 0; j < h; ++j)
			{
				const float value = *tmp;
				
				*tmp++ = a_min + (value - min) * s2 / s1;
			}
		}
	}
	
	void Heightfield::mapMaximum(float max)
	{
		FilterMapMaximum filter;
		filter.setMax(max);
		filter.apply(*this, *this);
	}

	void Heightfield::copyTo(Heightfield & dst) const
	{
		if (&dst == this)
			return;
			
	    dst.setSize(w, h);
	    
	    for (int i = 0; i < w; ++i)
	    {
	        const float * src_itr = height[i];
	              float * dst_itr = dst.height[i];
	        
	        for (int j = 0; j < h; ++j)
	        {
	            *dst_itr++ = *src_itr++;
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

	void DoubleBufferedHeightfield::clamp(float min, float max)
	{
		return heightfield[currentIndex].clamp(min, max);
	}
	
	void DoubleBufferedHeightfield::rerange(float min, float max)
	{
		return heightfield[currentIndex].rerange(min, max);
	}
	
	void DoubleBufferedHeightfield::mapMaximum(float max)
	{
		return heightfield[currentIndex].mapMaximum(max);
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
