#pragma once

#include <vector>

struct Slideshow
{
	struct PicInfo
	{
		PicInfo()
			: index(-1)
			, zoom1(0.f)
			, zoom2(0.f)
		{
		}

		int index;
		float zoom1;
		float zoom2;
	};

	std::vector<std::string> pics;

	float picTimer = 0.f;
	float picTimerRcp = 0.f;

	PicInfo picInfo[2];

	void tick(const float dt);
	void draw() const;
};
