#include <cmath>
#include <vector>
#include "framework.h"
#include "Options.h"

OPTION_DECLARE(float, GRAVITY, 500.f);
OPTION_DEFINE(float, GRAVITY, "Gravity");

OPTION_DECLARE(float, ICE_BOUNCE_X, .3f);
OPTION_DECLARE(float, ICE_BOUNCE_Y, .2f);
OPTION_DECLARE(float, ICE_FRIC_GROUND, .1f);
OPTION_DECLARE(int, ICE_FIRE_INTERVAL, 2);
OPTION_DECLARE(float, ICE_FIRE_SPEED, 700.f);
OPTION_DECLARE(float, ICE_FIRE_ANGLE, -8.f);
OPTION_DECLARE(float, ICE_FIRE_ANGLE_VAR, 4.f);
OPTION_DEFINE(float, ICE_BOUNCE_X, "Ice Bounce X");
OPTION_DEFINE(float, ICE_BOUNCE_Y, "Ice Bounce Y");
OPTION_DEFINE(float, ICE_FRIC_GROUND, "Ice Friction Ground");
OPTION_DEFINE(int, ICE_FIRE_INTERVAL, "Ice Fire Interval");
OPTION_DEFINE(float, ICE_FIRE_SPEED, "Ice Fire Speed");
OPTION_DEFINE(float, ICE_FIRE_ANGLE, "Ice Fire Angle");
OPTION_DEFINE(float, ICE_FIRE_ANGLE_VAR, "Ice Fire Angle Variance");

OPTION_DECLARE(int, EXPLOSION_FIRE_INTERVAL, 4);
OPTION_DECLARE(float, EXPLOSION_TIME, .2f);
OPTION_DECLARE(float, EXPLOSION_RADIUS1, 40.f);
OPTION_DECLARE(float, EXPLOSION_RADIUS2, 40.f);
OPTION_DEFINE(int, EXPLOSION_FIRE_INTERVAL, "Explosion Fire Interval");
OPTION_DEFINE(float, EXPLOSION_TIME, "Explosion Time");
OPTION_DEFINE(float, EXPLOSION_RADIUS1, "Explosion Radius 1");
OPTION_DEFINE(float, EXPLOSION_RADIUS2, "Explosion Radius 2");

static const float pi = 3.14159265358979323846f;

static void drawSprite(float x, float y, float scale, const char * name)
{
	Sprite(name).drawEx(x, y, 0.f, scale, scale);
}

static void drawCircle(float x, float y, float radius)
{
	const float length = pi * 2.f * radius;
	const int numSteps = std::ceilf(length / 10.f);
	const float step = 2.f * pi / numSteps;

	gxBegin(GL_POLYGON);
	for (int i = 0; i <= numSteps; ++i)
	{
		const float a = i * step;
		const float ax = std::cosf(a) * radius;
		const float ay = std::sinf(a) * radius;
		gxVertex2f(x + ax, y + ay);
	}
	gxEnd();
}

struct Plane
{
	float px;
	float py;
	float pd;

	const float dot(float x, float y, float d) const
	{
		return x * px + y * py + pd * d;
	}

	void draw()
	{
		const int axis = std::abs(px) < std::abs(py) ? 0 : 1;

		gxBegin(GL_POINTS);
		for (int i = 0; i < 800; i += 10)
		{
			const float x = axis == 0 ? i : 0;
			const float y = axis == 1 ? i : 0;
			const float d = dot(x, y, 1.f);

			const float projX = x - d * px;
			const float projY = y - d * py;

			gxVertex2f(projX, projY);
		}
		gxEnd();
	}
};

struct Scene
{
	std::vector<Plane> planes;
};

static Scene scene;

struct Projectile
{
	float x;
	float y;
	float vx;
	float vy;
	float bx;
	float by;

	void tick(float dt)
	{
		bx = ICE_BOUNCE_X;
		by = ICE_BOUNCE_Y;

		//

		vy += GRAVITY * dt;

		float newX = x + vx * dt;
		float newY = y + vy * dt;

		const float fricX = 0.f;
		const float fricY = 1.f;

		float fricAmount = 0.f;

		bool didBounce = false;

		for (auto & p : scene.planes)
		{
			const float d = p.dot(newX, newY, 1.f);

			if (d < 0.f)
			{
				const float vd = p.dot(vx, vy, 0.f);

				vx -= vd * p.px * (1.f + bx);
				vy -= vd * p.py * (1.f + by);

				newX -= (d - .001f) * p.px;
				newY -= (d - .001f) * p.py;

				fricAmount -= p.dot(fricX, fricY, 0.f);
			}
		}

		x = newX;
		y = newY;

		if (fricAmount != 0.f)
		{
			vx *= std::powf(ICE_FRIC_GROUND, dt);
		}
	}

	void draw()
	{
		setColor(colorGreen);
		drawRect(x - 5.f, y - 5.f, x + 5.f, y + 5.f);
	}
};

struct Explosion
{
	float x;
	float y;
	float t;
	float tRcp;

	void tick(float dt)
	{
		t -= dt * tRcp;
	}

	void draw()
	{
		if (t < 0.f)
		{
		}
		else if (t < .5f)
		{
			setColor(colorWhite);
			drawCircle(x, y, EXPLOSION_RADIUS1);
		}
		else
		{
			setColor(colorBlack);
			drawCircle(x, y, EXPLOSION_RADIUS2);
		}
	}
};

static std::vector<Projectile> ps;
static std::vector<Explosion> es;

int main(int argc, char * argv[])
{
	framework.fullscreen = false;
	framework.windowTitle = "EXPLOSIONS!";

	framework.init(0, 0, 640, 480);
	{
		// build scene

		{
			Plane plane;
			plane.px = -1.f;
			plane.py = 0.f;
			plane.pd = -(600 * plane.px + 0 * plane.py);
			scene.planes.push_back(plane);
		}

		{
			Plane plane;
			plane.px = 0.f;
			plane.py = -1.f;
			plane.pd = -(0 * plane.px + 400 * plane.py);
			scene.planes.push_back(plane);
		}

		for (int i = 0; i < 1; ++i)
		{
			Projectile p;
			p.x = 100.f;
			p.y = 100.f;
			p.vx = 100 + (rand() % 100);
			p.vy = 0.f;
			ps.push_back(p);
		}

		int fireFrame = 0;
		int explosionFrame = 0;

		while (!keyboard.isDown(SDLK_ESCAPE))
		{
			framework.process();

			const float dt = framework.timeStep;

			// todo : test ice/spray weapon physics and collision
			// use just a single ground and wall plane for collisions?
			// stuff to figure out:
			// spray speed, angle, gravity multiplier?, bounciness vs ground, bounciness vs wall,
			// disappear speed and behaviour, graphical appearence

			// todo : create explosions!

			if (mouse.isDown(BUTTON_LEFT))
			{
				if ((fireFrame % ICE_FIRE_INTERVAL) == 0)
				{
					const float angleVar = (rand() % 1000) / 999.f - .5f;
					const float angle = ICE_FIRE_ANGLE + ICE_FIRE_ANGLE_VAR * angleVar;
					Projectile p;
					p.x = mouse.x;
					p.y = mouse.y;
					p.vx = std::cos(angle * pi / 180.f) * ICE_FIRE_SPEED;
					p.vy = std::sin(angle * pi / 180.f) * ICE_FIRE_SPEED;
					ps.push_back(p);
				}

				fireFrame++;
			}
			else
				fireFrame = 0;

			if (mouse.isDown(BUTTON_RIGHT))
			{
				if ((explosionFrame % EXPLOSION_FIRE_INTERVAL) == 0)
				{
					Explosion e;
					e.x = mouse.x;
					e.y = mouse.y;
					e.t = 1.f;
					e.tRcp = 1.f / EXPLOSION_TIME;
					es.push_back(e);
				}

				explosionFrame++;
			}
			else
				explosionFrame = 0;

			for (auto & p : ps)
				p.tick(dt);

			for (auto & e : es)
				e.tick(dt);

			framework.beginDraw(31, 31, 31, 0);
			{
				for (auto & p : scene.planes)
				{
					setColor(colorWhite);
					p.draw();
				}

				for (auto & p : ps)
					p.draw();

				for (auto & e : es)
					e.draw();
			}
			framework.endDraw();
		}
	}
	framework.shutdown();

	return 0;
}
