#pragma once

namespace lgen
{
	struct Filter;
	
	struct Heighfield
	{
		int ** height = nullptr;
		int w = 0;
		int h = 0;

		virtual ~Heighfield();

		bool setSize(int w, int h);
		void clear();
		
		int getHeight(int x, int y) const
		{
			return height[x][y];
		}

		void clamp(int min, int max);
		void rerange(int min, int max);
		void copyTo(Heighfield & dst) const;
		bool getSizePowers(int & pw, int & ph) const;
	};
	
	struct DoubleBufferedHeightfield
	{
		Heighfield heightfield[2];
		int currentIndex = 0;
		
		bool setSize(int w, int h);
		void clear();
		
		Heighfield & get()
		{
			return heightfield[currentIndex];
		}
		
		int getHeight(int x, int y) const
		{
			return heightfield[currentIndex].getHeight(x, y);
		}

		void clamp(int min, int max);
		void rerange(int min, int max);
		void copyTo(Heighfield & dst) const;
		bool getSizePowers(int & pw, int & ph) const;
		
		void swapBuffers();
		bool applyFilter(Filter & filter);
	};
}
