#pragma once

class Rect
{
public:
	inline Rect()
	{
	}

	inline Rect(int min_x, int min_y, int max_x, int max_y)
	{
		min[0] = min_x;
		min[1] = min_y;
		max[0] = max_x;
		max[1] = max_y;
	}

	inline void Set(int min_x, int min_y, int max_x, int max_y)
	{
		min[0] = min_x;
		min[1] = min_y;
		max[0] = max_x;
		max[1] = max_y;
	}

	inline void Merge(const Rect& rect)
	{
		for (int i = 0; i < 2; ++i)
		{
			if (rect.min[i] < min[i])
				min[i] = rect.min[i];
			if (rect.max[i] > max[i])
				max[i] = rect.max[i];
		}
	}

	inline bool Clip(const Rect& rect)
	{
		for (int i = 0; i < 2; ++i)
		{
			if (min[i] < rect.min[i])
				min[i] = rect.min[i];
			if (max[i] > rect.max[i])
				max[i] = rect.max[i];

			if (min[i] > max[i])
				return false;
		}

		return true;
	}

	inline int Area()
	{
		int w = max[0] - min[0] + 1;
		int h = max[1] - min[1] + 1;

		return w * h;
	}

	union
	{
		struct
		{
			int min[2];
			int max[2];
		};
		struct
		{
			int x1, y1;
			int x2, y2;
		};
	};
};
