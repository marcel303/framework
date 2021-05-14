#pragma once

namespace lgen
{
	struct Filter;
	
	struct Heightfield
	{
		float ** height = nullptr;
		int w = 0;
		int h = 0;

		virtual ~Heightfield();

		bool setSize(int w, int h);
		void clear();
		
		float getHeight(int x, int y) const
		{
			return height[x][y];
		}

		void clamp(float min, float max);
		void rerange(float min, float max);
		void mapMaximum(float max);
		void copyTo(Heightfield & dst) const;
		bool getSizePowers(int & pw, int & ph) const;
	};
	
	//
	
	struct DoubleBufferedHeightfield
	{
		Heightfield heightfield[2];
		int currentIndex = 0;
		
		bool setSize(int w, int h);
		void clear();
		
		Heightfield & get()
		{
			return heightfield[currentIndex];
		}
		
		float getHeight(int x, int y) const
		{
			return heightfield[currentIndex].getHeight(x, y);
		}

		void clamp(float min, float max);
		void rerange(float min, float max);
		void mapMaximum(float max);
		void copyTo(Heightfield & dst) const;
		bool getSizePowers(int & pw, int & ph) const;
		
		void swapBuffers();
		bool applyFilter(Filter & filter);
	};
}
