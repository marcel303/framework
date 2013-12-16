#pragma once

class Vec2
{
public:
	union
	{
		struct
		{
			float x;
			float y;
		};
		float v[2];
	};
};