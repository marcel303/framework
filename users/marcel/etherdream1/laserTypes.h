#pragma once

struct LaserPoint
{
	float x;
	float y;

	float r;
	float g;
	float b;

	void set(
		const float x,
		const float y,
		const float r = 1.f,
		const float g = 1.f,
		const float b = 1.f)
	{
		this->x = x;
		this->y = y;
		this->r = r;
		this->g = g;
		this->b = b;
	}
	
	static inline float interpFloat(const float v1, const float v2, const float t)
	{
		return v1 * (1.f - t) + v2 * t;
	}
	
	void interpTo(const LaserPoint & other, const float t, LaserPoint & out) const
	{
		out.x = interpFloat(x, other.x, t);
		out.y = interpFloat(y, other.y, t);
		out.r = interpFloat(r, other.r, t);
		out.g = interpFloat(g, other.g, t);
		out.b = interpFloat(b, other.b, t);
	}
};
