#pragma once

#include <stdint.h>

namespace Calc
{
	inline uint8_t Saturate8(int v)
	{
		if (v < 0)
			return 0;
		else if (v > 255)
			return 255;
		else
			return v;
	}
	
	inline double Saturate(double v)
	{
		if (v < 0.0)
			return 0.0;
		else if (v > 1.0)
			return 1.0;
		else
			return v;
	}
	
	inline uint8_t dtoc(double v)
	{
		return Saturate8((int)(v * 256.0));
	}
}

class Pixel
{
public:
	inline Pixel()
	{
		r = 0;
		g = 255;
		b = 0;
		a = 255;
	}
	
	inline Pixel(double _r, double _g, double _b, double _a)
	{
		r = Calc::dtoc(_r);
		g = Calc::dtoc(_g);
		b = Calc::dtoc(_b);
		a = Calc::dtoc(_a);
	}
	
	uint8_t r, g, b, a;
};

class Surface
{
public:
	Surface();
	~Surface();
	
	void Setup(int sx, int sy);
	
	inline int Sx_get() const
	{
		return mSx;
	}
	
	inline int Sy_get() const
	{
		return mSy;
	}
	
	void Pixel_set(int x, int y, Pixel pixel);
	
	uint8_t* Buffer_get()
	{
		return (uint8_t*)mPixels;
	}
	
private:
	Pixel* mPixels;
	int mSx;
	int mSy;
};
