#pragma once

// TODO: Fixed point coord val?
namespace Paint
{
	typedef struct
	{
		union
		{
			float p[2];
			struct
			{
				float x, y;
			};
		};
	} Coord;
};
