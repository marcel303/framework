#include "framework.h"
#include <assert.h>

#define GFX_SX 640
#define GFX_SY 480

struct Particle
{
	Vec2 p;
	Vec2 v;
	Vec2 a;
};

struct Shape
{
	std::vector<Vec3> planes;
	
	void constrainPosition(Vec2 & p)
	{
		for (const auto & plane : planes)
		{
			const float d = Vec3(p[0], p[1], 1.f) * plane;
			
			if (d > 0.f)
				p -= Vec2(plane[0], plane[1]) * d * .5f;
		}
	}
};

static void clipPoly(const std::vector<Vec2> & poly_in, const Vec3 & plane, std::vector<Vec2> & poly_out)
{
	const float eps = std::numeric_limits<float>::epsilon();
	
	for (size_t i = 0; i < poly_in.size(); ++i)
	{
		const Vec2 & p1 = poly_in[i];
		const Vec2 & p2 = (i + 1 == poly_in.size()) ? poly_in[0] : poly_in[i + 1];
		
		const float d1 = Vec3(p1[0], p1[1], 1.f) * plane;
		const float d2 = Vec3(p2[0], p2[1], 1.f) * plane;
		
		if (d1 < +eps && d2 < +eps)
		{
			poly_out.push_back(p1);
		}
		else if (d1 > -eps && d2 > -eps)
		{
			
		}
		else
		{
			const float t = -d1 / (d2 - d1);
			
			if (d1 < +eps)
			{
				assert(d2 > 0.f);
				
				poly_out.push_back(p1);
				poly_out.push_back(p1 + (p2 - p1) * t);
			}
			else
			{
				assert(d1 > 0.f);
				
				poly_out.push_back(p1 + (p2 - p1) * t);
				poly_out.push_back(p2);
			}
		}
	}
}

void drawShape(const Shape & shape)
{
	std::vector<Vec2> poly_in;
	std::vector<Vec2> poly_out;
	
	const float inf = 1e3f;
	
	poly_in.push_back(Vec2(-inf, -inf));
	poly_in.push_back(Vec2(+inf, -inf));
	poly_in.push_back(Vec2(+inf, +inf));
	poly_in.push_back(Vec2(-inf, +inf));
	
	for (const auto & plane : shape.planes)
	{
		clipPoly(poly_in, plane, poly_out);
		
		std::swap(poly_in, poly_out);
		
		poly_out.clear();
	}
	
	const auto & poly = poly_in;
	
	gxBegin(GL_TRIANGLE_FAN);
	{
		for (const auto & vertex : poly)
			gxVertex2f(vertex[0], vertex[1]);
	}
	gxEnd();
}

int main(int argc, char * argv[])
{
	if (framework.init(GFX_SX, GFX_SY))
	{
		std::vector<Particle> particles;
		
		particles.resize(100);
		
		for (auto & particle : particles)
			particle.p.Set(random(-2.f, +2.f), random(-2.f, +2.f));
		
		Shape shape;
		
		float angle = 0.f;
		
		while (angle < 360.f)
		{
			const float nx = cosf(angle * M_PI / 180.f);
			const float ny = sinf(angle * M_PI / 180.f);
			const float d = -1.f;
			
			shape.planes.push_back(Vec3(nx, ny, d));
			
			angle += 60.f;
		}
		
		while (!framework.quitRequested)
		{
			framework.process();

			if (keyboard.wentDown(SDLK_ESCAPE))
				framework.quitRequested = true;
			
			if (keyboard.isDown(SDLK_SPACE))
			{
				for (auto & particle : particles)
				{
					particle.p.Set(random(-2.f, +2.f), random(-2.f, +2.f));
					particle.v.Set(random(-1.f, +1.f), random(-1.f, +1.f));
				}
			}
			
			for (int i = 0; i < 10; ++i)
			{
			const float dt = framework.timeStep / 10.f;
			
			for (size_t i = 0; i < particles.size(); ++i)
			{
				auto & p1 = particles[i];
				
				p1.a.SetZero();
				
				for (size_t j = 0; j < particles.size(); ++j)
				{
					if (i == j)
						continue;
					
					auto & p2 = particles[j];
					
					const Vec2 delta = p2.p - p1.p;
					const float distance = delta.CalcSize();
					
					if (distance > 0.f)
					{
						const Vec2 f = (delta / distance) / (distance * distance);
						
						p1.a -= f;
					}
				}
			}
			
			for (auto & particle : particles)
			{
				particle.v *= powf(.1f, dt);
				
				particle.v += particle.a * dt;
				particle.p += particle.v * dt;
				
				shape.constrainPosition(particle.p);
			}
			}
			
			framework.beginDraw(0, 0, 0, 0);
			{
				gxTranslatef(GFX_SX/2, GFX_SY/2, 0);
				gxScalef(30, 30, 1);
				
				setColor(colorBlue);
				drawShape(shape);
				
				setColor(colorGreen);
				gxBegin(GL_POINTS);
				for (const auto & particle : particles)
					gxVertex2f(particle.p[0], particle.p[1]);
				gxEnd();
			}
			framework.endDraw();
		}

		framework.shutdown();
	}

	return 0;
}
