#pragma once

#include <math.h>
#include <stdlib.h>

namespace Calc
{
	inline double Random()
	{
		return (rand() % 1000000) / 1000000.0;
	}
	
	inline double Random(double max)
	{
		return Random() * max;
	}
	
	inline double Random(double min, double max)
	{
		double t = Random();
		
		return min + (max - min) * t;
	}
	
	inline double RandomS(double scale)
	{
		return (Random() - 0.5) * 2.0 * scale;
	}
	
	inline void RandomA(double scaleMin, double scaleMax, double& oX, double&oY)
	{
		double angle = Random(M_PI * 2.0);
		double scale = Random(scaleMin, scaleMax);

		oX = cos(angle) * scale;
		oY = sin(angle) * scale;
	}
}

class Vec
{
public:
	Vec()
	{
		mV[0] = mV[1] = 0.0;
	}
	
	Vec(double x, double y)
	{
		mV[0] = x;
		mV[1] = y;
	}
	
	Vec(const Vec& other)
	{
		mV[0] = other.mV[0];
		mV[1] = other.mV[1];
	}
	
	double L() const
	{
		return sqrt(mV[0] * mV[0] + mV[1] * mV[1]);
	}
	
	double L2() const
	{
		return mV[0] * mV[0] + mV[1] * mV[1];
	}
	
	Vec D() const
	{
		double l = L();
		
		if (l == 0.0)
			return Vec(1.0, 0.0);
		else
			return Vec(mV[0] / l, mV[1] / l);
	}
	
	Vec N() const
	{
		double l = L();
		
		if (l == 0.0)
			return Vec(0.0, 1.0);
		else
			return Vec(-mV[1] / l, +mV[0] / l);
	}
	
	void AddS(Vec& v, double s)
	{
		mV[0] += v.mV[0] * s;
		mV[1] += v.mV[1] * s;
	}
	
	inline Vec operator-(const Vec& other) const
	{
		return Vec(mV[0] - other.mV[0], mV[1] - other.mV[1]);
	}
	
	inline double& operator[](int index)
	{
		return mV[index];
	}
	
private:
	double mV[2];
};

