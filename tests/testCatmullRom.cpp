/*
	Copyright (C) 2017 Marcel Smit
	marcel303@gmail.com
	https://www.facebook.com/marcel.smit981

	Permission is hereby granted, free of charge, to any person
	obtaining a copy of this software and associated documentation
	files (the "Software"), to deal in the Software without
	restriction, including without limitation the rights to use,
	copy, modify, merge, publish, distribute, sublicense, and/or
	sell copies of the Software, and to permit persons to whom the
	Software is furnished to do so, subject to the following
	conditions:

	The above copyright notice and this permission notice shall be
	included in all copies or substantial portions of the Software.

	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
	EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
	OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
	NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
	HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
	WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
	FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
	OTHER DEALINGS IN THE SOFTWARE.
*/

#include "framework.h"
#include "testBase.h"
#include <iostream>
#include <limits>

extern const int GFX_SX;
extern const int GFX_SY;

struct CubicPoly
{
	float c0, c1, c2, c3;
	
	float eval(float t)
	{
		float t2 = t*t;
		float t3 = t2 * t;
		return c0 + c1*t + c2*t2 + c3*t3;
	}
};

/*
 * Compute coefficients for a cubic polynomial
 *   p(s) = c0 + c1*s + c2*s^2 + c3*s^3
 * such that
 *   p(0) = x0, p(1) = x1
 *  and
 *   p'(0) = t0, p'(1) = t1.
 */
void InitCubicPoly(float x0, float x1, float t0, float t1, CubicPoly &p)
{
    p.c0 = x0;
    p.c1 = t0;
    p.c2 = -3*x0 + 3*x1 - 2*t0 - t1;
    p.c3 = 2*x0 - 2*x1 + t0 + t1;
}

// standard Catmull-Rom spline: interpolate between x1 and x2 with previous/following points x1/x4
// (we don't need this here, but it's for illustration)
void InitCatmullRom(float x0, float x1, float x2, float x3, CubicPoly &p)
{
	// Catmull-Rom with tension 0.5
    InitCubicPoly(x1, x2, 0.5f*(x2-x0), 0.5f*(x3-x1), p);
}

// compute coefficients for a nonuniform Catmull-Rom spline
void InitNonuniformCatmullRom(float x0, float x1, float x2, float x3, float dt0, float dt1, float dt2, CubicPoly &p)
{
    // compute tangents when parameterized in [t1,t2]
    float t1 = (x1 - x0) / dt0 - (x2 - x0) / (dt0 + dt1) + (x2 - x1) / dt1;
    float t2 = (x2 - x1) / dt1 - (x3 - x1) / (dt1 + dt2) + (x3 - x2) / dt2;

    // rescale tangents for parametrization in [0,1]
    t1 *= dt1;
    t2 *= dt1;

    InitCubicPoly(x1, x2, t1, t2, p);
}

struct Vec2D
{
	Vec2D(float _x, float _y) : x(_x), y(_y) {}
	float x, y;
};

float VecDistSquared(const Vec2D& p, const Vec2D& q)
{
	float dx = q.x - p.x;
	float dy = q.y - p.y;
	return dx*dx + dy*dy;
}

void InitCentripetalCR(const Vec2D& p0, const Vec2D& p1, const Vec2D& p2, const Vec2D& p3,
	CubicPoly &px, CubicPoly &py)
{
    float dt0 = powf(VecDistSquared(p0, p1), 0.25f);
    float dt1 = powf(VecDistSquared(p1, p2), 0.25f);
    float dt2 = powf(VecDistSquared(p2, p3), 0.25f);

	// safety check for repeated points
    if (dt1 < 1e-4f)    dt1 = 1.0f;
    if (dt0 < 1e-4f)    dt0 = dt1;
    if (dt2 < 1e-4f)    dt2 = dt1;

	InitNonuniformCatmullRom(p0.x, p1.x, p2.x, p3.x, dt0, dt1, dt2, px);
	InitNonuniformCatmullRom(p0.y, p1.y, p2.y, p3.y, dt0, dt1, dt2, py);
}

static void snapPointHV(std::vector<Vec2D> & points, float & x, float & y)
{
	float minDx = std::numeric_limits<float>::max();
	float minDy = std::numeric_limits<float>::max();
	
	for (size_t i = 0; i < points.size(); ++i)
	{
		const float dx = x - points[i].x;
		const float dy = y - points[i].y;
		
		if (std::abs(dx) < std::abs(minDx))
			minDx = dx;
		if (std::abs(dy) < std::abs(minDy))
			minDy = dy;
	}
	
	if (std::abs(minDx) < 10.f)
		x -= minDx;
	if (std::abs(minDy) < 10.f)
		y -= minDy;
}

void testCatmullRom()
{
	std::vector<Vec2D> points;
	
	do
	{
		framework.process();
		
		//
		
		float x = mouse.x;
		float y = mouse.y;
		
		const bool snap = keyboard.isDown(SDLK_LSHIFT) || keyboard.isDown(SDLK_RSHIFT);
		
		if (snap)
		{
			snapPointHV(points, x, y);
		}
		
		if (mouse.wentDown(BUTTON_LEFT))
		{
			points.push_back(Vec2D(x, y));
		}
		
		if (mouse.isDown(BUTTON_RIGHT))
		{
			points.push_back(Vec2D(x, y));
		}
		
		//
		
		framework.beginDraw(0, 0, 0, 0);
		{
			hqBegin(HQ_FILLED_CIRCLES, true);
			{
				const int numSteps = 100;
				
				for (size_t i = 0; i + 4 <= points.size(); ++i)
				{
					auto & p0 = points[i + 0];
					auto & p1 = points[i + 1];
					auto & p2 = points[i + 2];
					auto & p3 = points[i + 3];
					
					CubicPoly px;
					CubicPoly py;
					
				#if 1
					InitCentripetalCR(
						p0, p1, p2, p3,
						px, py);
				#else
					InitCatmullRom(p0.x, p1.x, p2.x, p3.x, px);
					InitCatmullRom(p0.y, p1.y, p2.y, p3.y, py);
				#endif
					
					for (int i = 0; i < numSteps; ++i)
					{
						const auto x = px.eval(i / float(numSteps - 1));
						const auto y = py.eval(i / float(numSteps - 1));
						
						setColor(colorWhite);
						hqFillCircle(x, y, 3.f);
					}
				}
			}
			hqEnd();
			
			hqBegin(HQ_FILLED_CIRCLES, true);
			{
				for (size_t i = 0; i + 1 <= points.size(); ++i)
				{
					setColor(colorGreen);
					hqFillCircle(points[i].x, points[i].y, 5.f);
				}
			}
			hqEnd();
			
			if (snap)
			{
				hqBegin(HQ_LINES);
				{
					const float strokeSize = 2.f;
					
					setColor(colorRed);
					hqLine(0, y, strokeSize, GFX_SX, y, strokeSize);
					hqLine(x, 0, strokeSize, x, GFX_SY, strokeSize);
				}
				hqEnd();
			}
			
			drawTestUi();
		}
		framework.endDraw();
	} while (tickTestUi());
	
	exit(0);
}
