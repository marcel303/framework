#pragma once

#include <cmath>

struct FollowValue
{
	float value;
	float targetValue;
	float followPerSecond;

	FollowValue(const float initialValue, const float _followPerSecond)
		: value(initialValue)
		, targetValue(initialValue)
		, followPerSecond(_followPerSecond)
	{
	}

	void tick(const float dt)
	{
		const float a1 = std::powf(1.f - followPerSecond, dt);
		const float a2 = 1.f - a1;

		value = value * a1 + targetValue * a2;
	}
};
