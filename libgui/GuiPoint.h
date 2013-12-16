#pragma once

namespace Gui
{
	class Point
	{
	public:
		Point()
		{
			x = 0;
			y = 0;
		}
		Point(int x, int y)
		{
			this->x = x;
			this->y = y;
		}

		union
		{
			struct
			{
				int x;
				int y;
			};
			struct
			{
				int min;
				int max;
			};
			int values[2];
		};
	};
};
