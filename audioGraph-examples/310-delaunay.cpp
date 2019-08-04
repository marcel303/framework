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

#include "delaunay/delaunay.h"
#include "delaunay/triangle.h"
#include "delaunay/vector2.h"
#include "framework.h"

#include "ui.h"

const int GFX_SX = 1300;
const int GFX_SY = 760;

static float dot(const Vec2 & V1, const Vec2 & V2)
{
	return V1 * V2;
}

// @see http://blackpawn.com/texts/pointinpoly/
static bool baryPointInTriangle(
	const Vec2 & A,
	const Vec2 & B,
	const Vec2 & C,
	const Vec2 & P,
	const float eps,
	float & baryU, float & baryV)
{
	// Compute vectors
	const Vec2 v0 = C - A;
	const Vec2 v1 = B - A;
	const Vec2 v2 = P - A;

	// Compute dot products
	const float dot00 = dot(v0, v0);
	const float dot01 = dot(v0, v1);
	const float dot02 = dot(v0, v2);
	const float dot11 = dot(v1, v1);
	const float dot12 = dot(v1, v2);

	// Compute barycentric coordinates
	const float invDenom = 1.f / (dot00 * dot11 - dot01 * dot01);
	const float u = (dot11 * dot02 - dot01 * dot12) * invDenom;
	const float v = (dot00 * dot12 - dot01 * dot02) * invDenom;

	// Check if point is in triangle
	if ((u >= 0.f - eps) && (v >= 0.f - eps) && (u + v < 1.f + eps))
	{
		baryU = u;
		baryV = v;
		
		return true;
	}
	else
	{
		return false;
	}
}

int main(int argc, char * argv[])
{
#if defined(CHIBI_RESOURCE_PATH)
	changeDirectory(CHIBI_RESOURCE_PATH);
#else
	changeDirectory(SDL_GetBasePath());
#endif

#if FULLSCREEN
	framework.fullscreen = true;
#endif

	//framework.waitForEvents = true;
	
	if (!framework.init(GFX_SX, GFX_SY))
		return -1;

	initUi();

	//const int numPoints = 20;
	
	typedef Vector2<float> Pointf;
	typedef Triangle<float> Trianglef;
	typedef Edge<float> Edgef;
	
	Delaunay<float> triangulation;
	std::vector<Trianglef> triangles;
	std::vector<Edgef> edges;
	
	bool randomize = true;
	
	auto toScreen = [](Pointf in)
	{
		in.x = (in.x + 180.f) / 360.f * GFX_SX;
		in.y = (in.y + 90.f) / 180.f * GFX_SY;
		
		return in;
	};
	
	auto toPolar = [](Pointf in)
	{
		in.x = in.x / GFX_SX * 360.f - 180.f;
		in.y = in.y / GFX_SY * 180.f - 90.f;
		
		return in;
	};
	
	do
	{
		framework.process();
		
		//
		
		if (keyboard.wentDown(SDLK_r))
		{
			randomize = true;
		}
		
		const Pointf polar = toPolar(Pointf(mouse.x, mouse.y));
		
		//
		
		if (randomize)
		{
			randomize = false;
			
			//
			
			std::vector<Pointf> points;
			
			for (int azimuth = -180; azimuth <= 180; azimuth += 5)
			{
				points.push_back(Pointf(azimuth, -90));
			}
			
			for (int elevation = -40; elevation <= 90; elevation += 10)
			{
				for (int azimuth = 0; azimuth <= 180; azimuth += 5)
				{
					Pointf p(azimuth, elevation);
					
					points.push_back(p);
					
					if (azimuth != 0)
					{
						Pointf pMirrored(-azimuth, elevation);
						
						points.push_back(pMirrored);
					}
				}
			}
			
			/*
			points.push_back(Pointf(0.f,    0.f   ));
			points.push_back(Pointf(GFX_SX, 0.f   ));
			points.push_back(Pointf(0.f,    GFX_SY));
			points.push_back(Pointf(GFX_SX, GFX_SY));
			
			for (int i = 0; i < numPoints; ++i)
			{
				Pointf p;
				p.x = random<float>(0.f, GFX_SX);
				p.y = random<float>(0.f, GFX_SY);
				points.push_back(p);
			}
			*/
			
			triangulation = Delaunay<float>();
			
			triangles = triangulation.triangulate(points);
			edges = triangulation.getEdges();
		}
		
		// find active triangle
		
		Trianglef * activeTriangle = nullptr;
		float baryU = 0.f;
		float baryV = 0.f;
		
		const float eps = .001f;
		
		for (auto & triangle : triangles)
		{
			const Vec2 p1(triangle.p1.x, triangle.p1.y);
			const Vec2 p2(triangle.p2.x, triangle.p2.y);
			const Vec2 p3(triangle.p3.x, triangle.p3.y);
			const Vec2 p(polar.x, polar.y);
			
			if (baryPointInTriangle(p1, p2, p3, p, eps, baryU, baryV))
			{
				activeTriangle = &triangle;
			}
		}
		
		//
		
		framework.beginDraw(0, 0, 0, 0);
		{
			hqBegin(HQ_FILLED_TRIANGLES);
			{
				for (auto & triangle : triangles)
				{
					Pointf p1 = toScreen(triangle.p1);
					Pointf p2 = toScreen(triangle.p2);
					Pointf p3 = toScreen(triangle.p3);
					
					if (&triangle == activeTriangle)
					{
						setColor(colorYellow);
					}
					else
					{
						setColorf(
							p1.x / GFX_SX,
							p1.y / GFX_SY,
							0.f);
					}
					
					hqFillTriangle(
						p2.x, p2.y,
						p1.x, p1.y,
						p3.x, p3.y);
				}
			}
			hqEnd();
			
			setFont("calibri.ttf");
			setColor(colorWhite);
			drawText(mouse.x, mouse.y, 16, 0, +1, "%d, %d", int(polar.x), int(polar.y));
			
			if (activeTriangle != nullptr)
			{
				drawText(mouse.x, mouse.y + 20, 16, 0, +1, "%.2f, %.2f, %.2f", baryU, baryV, 1.f - baryU - baryV);
			}
		}
		framework.endDraw();
	} while (!keyboard.wentDown(SDLK_ESCAPE));
	
	framework.shutdown();

	return 0;
}
