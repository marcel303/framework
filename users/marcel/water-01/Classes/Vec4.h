#pragma once

class Vec4
{
public:
	Vec4()
	{
		w = 1.0f;
	}
	
	union
	{
		struct
		{
			float x;
			float y;
			float z;
			float w;
		};
		float v[4];
	};
};