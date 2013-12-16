#pragma once

class Color
{
public:
	Color(float _r, float _g, float _b) :
		r(_r),
		g(_g),
		b(_b),
		a(1.0f)
	{
	}

	Color(float _r, float _g, float _b, float _a) :
		r(_r),
		g(_g),
		b(_b),
		a(_a)
	{
	}

	float r, g, b, a;
};

class Render
{
public:
	virtual void Init(int sx, int sy) = 0;
	virtual void MakeCurrent() = 0;
	virtual void Clear() = 0;
	virtual void Present() = 0;
	virtual void Fade(int amount) = 0;

	virtual void Point(float x, float y, Color color) = 0;
	virtual void Line(float x1, float y1, float x2, float y2, Color color) = 0;
	virtual void Circle(float x, float y, float r, Color color) = 0;
	virtual void Arc(float x, float y, float angle1, float angle2, float r, Color color) = 0;
	virtual void Quad(float x1, float y1, float x2, float y2, Color color) = 0;
	virtual void QuadTex(float x1, float y1, float x2, float y2, Color color, float u1, float v1, float u2, float v2) = 0;
	virtual void Text(float x, float y, Color color, const char* format, ...) = 0;
};

extern Render* gRender;
