#pragma once

#include "render.h"

class RenderGL : public Render
{
public:
	virtual void Init(int sx, int sy);
	virtual void MakeCurrent();
	virtual void Clear();
	virtual void Present();
	virtual void Fade(int amount);

	virtual void Point(float x, float y, Color color);
	virtual void Line(float x1, float y1, float x2, float y2, Color color);
	virtual void Circle(float x, float y, float r, Color color);
	virtual void Arc(float x, float y, float angle1, float angle2, float r, Color color);
	virtual void Quad(float x1, float y1, float x2, float y2, Color color);
	virtual void QuadTex(float x1, float y1, float x2, float y2, Color color, float u1, float v1, float u2, float v2);
	virtual void Text(float x, float y, Color color, const char* format, ...);

private:
	void SetupMatrices();

	struct SDL_Surface* mSurface;
};
