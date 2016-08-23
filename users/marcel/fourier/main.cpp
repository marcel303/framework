#include "Calc.h"
#include "framework.h"
#include "image.h"

#define GFX_SX 1920
#define GFX_SY 1080

#define TEST_BEZIER 0
#define TEST_FLOWPARTICLES 0
#define TEST_FIREWORKS 0
#define TEST_TRACERS 1

#if TEST_BEZIER

#include "BezierPath.h"
#include "Calc.h"
#include "Ease.h"

float evalPoint(float n1, float n2, float t)
{
	float diff = n2 - n1;

	return n1 + (diff * t);
}

// todo : simulate control points based on gravity, gravity well, sin/cos, ..

const static float kInitialDuration = 6.f;

struct BezierTest
{
	const static int kNumNodes = 7;
	const static int kNumLines = 10;

	float timeLeft;
	float duration;

	struct Node
	{
		BezierNode bezierNode;
		Vec2F posOffset;
		Vec2F tanOffset;
		Vec2F posSpeed;
		Vec2F tanSpeed;
		Vec2F currentPosition;
		Vec2F desiredPosition;
	};

	struct Line
	{
		Node nodes[kNumNodes];
	};

	Line lines[kNumLines];
	Line linesCopy[kNumLines];

	Color color;

	BezierTest()
		: timeLeft(0.f)
		, duration(0.f)
		, color(colorWhite)
	{
	}

	void tick(const float dt)
	{
		if (timeLeft == 0.f)
		{
			timeLeft = kInitialDuration;
			duration = kInitialDuration;

			//color = Color::fromHSL(random(0.f, 1.f), 1.f, .5f);
			color = colorWhite;

			Line baseLine;

			for (int i = 0; i < kNumNodes; ++i)
			{
				const float s = .4f;
				const float t = .4f;

				baseLine.nodes[i].desiredPosition[0] = random(-s, +s) + .3f;
				baseLine.nodes[i].desiredPosition[1] = random(-s, +s);

				if (i == 0)
					baseLine.nodes[i].desiredPosition[0] -= 1.f;
				if (i >= 1)
					baseLine.nodes[i].desiredPosition += baseLine.nodes[i - 1].desiredPosition;

				if (false)
				{
					const float a = i / float(kNumNodes - 1) * Calc::m2PI;
					const float d = random(-.2f, 1.f);
					baseLine.nodes[i].desiredPosition[0] = std::cos(a) * d;
					baseLine.nodes[i].desiredPosition[1] = std::sin(a) * d;
				}

				baseLine.nodes[i].currentPosition = baseLine.nodes[i].desiredPosition;

				baseLine.nodes[i].bezierNode.m_Position = baseLine.nodes[i].desiredPosition;

				//

				baseLine.nodes[i].bezierNode.m_Tangent[0][0] = random(-t, +t);
				baseLine.nodes[i].bezierNode.m_Tangent[0][1] = random(-t, +t);
				baseLine.nodes[i].bezierNode.m_Tangent[1] = -baseLine.nodes[i].bezierNode.m_Tangent[0];
				
				//

				const float vP = .6f / kInitialDuration;
				const float vT = .4f / kInitialDuration;

				baseLine.nodes[i].posSpeed[0] = random(-vP, +vP);
				baseLine.nodes[i].posSpeed[1] = random(-vP, +vP);

				baseLine.nodes[i].tanSpeed[0] = random(-vT, +vT);
				baseLine.nodes[i].tanSpeed[1] = random(-vT, +vT);
			}
			
			// make lines

			for (int i = 0; i < kNumLines; ++i)
			{
				linesCopy[i] = lines[i];

				lines[i] = baseLine;

				for (int j = 0; j < kNumNodes; ++j)
				{
					const float t = (i / float(kNumNodes - 1) - .5f) * 2.f;

					lines[i].nodes[j].posOffset = lines[i].nodes[j].posSpeed.Normal() * t * 0.07f;
				}
			}
		}

		//

		const float a = 1.f - (timeLeft / duration);
		const float t = duration - timeLeft;

		//

		for (int i = 0; i < kNumLines; ++i)
		{
			for (int j = 0; j < kNumNodes; ++j)
			{
				//const float aa = .9f;
				//const float bb = 1.f - std::powf(aa, dt);

				//lines[i].nodes[j].currentPosition[0] = Calc::Lerp(lines[i].nodes[j].currentPosition[0], lines[i].nodes[j].desiredPosition[0], bb);
				//lines[i].nodes[j].currentPosition[1] = Calc::Lerp(lines[i].nodes[j].currentPosition[1], lines[i].nodes[j].desiredPosition[1], bb);

				lines[i].nodes[j].currentPosition = lines[i].nodes[j].desiredPosition;

				//lines[i].nodes[j].currentPosition[1] += sin(j + t * 4.f) * .1f;
				
				//

				lines[i].nodes[j].posOffset += lines[i].nodes[j].posSpeed * dt;
				lines[i].nodes[j].tanOffset += lines[i].nodes[j].tanSpeed * dt;

				lines[i].nodes[j].bezierNode.m_Position = lines[i].nodes[j].currentPosition + lines[i].nodes[j].posOffset;
			}
		}

		if (keyboard.wentDown(SDLK_SPACE))
		{
			for (int j = 0; j < kNumNodes; ++j)
			{
				if ((rand() % 4) == 0)
				{
					for (int i = 0; i < kNumLines; ++i)
					{
						Node & node = lines[i].nodes[j];

						node.posSpeed *= 2.f;
					}
				}
			}
		}

		//

		timeLeft = Calc::Max(0.f, timeLeft - dt);
	}

	void draw(const float alpha) const
	{
		const float t = 1.f - (timeLeft / duration);

		//const float a = pow(sin(t * Calc::mPI), .5f) * alpha;
		const float a = 1.f;

		gxPushMatrix();
		{
			gxTranslatef(GFX_SX/2, GFX_SY/2, 0.f);
			gxScalef(500.f, 500.f, 1.f);
			gxColor4f(color.r, color.g, color.b, a);

			for (int i = 0; i < kNumLines; ++i)
			{
				const Line & oldLine = linesCopy[i];
				const Line & newLine = lines[i];

				BezierNode nodes[kNumNodes];

				for (int j = 0; j < kNumNodes; ++j)
				{
					const Node & oldNode = oldLine.nodes[j];
					const Node & newNode = newLine.nodes[j];

					const Vec2F oldPos = oldNode.currentPosition + oldNode.posOffset;
					const Vec2F newPos = newNode.currentPosition + newNode.posOffset;

					const Vec2F oldTan = oldNode.bezierNode.m_Tangent[0] + oldNode.tanOffset;
					const Vec2F newTan = newNode.bezierNode.m_Tangent[0] + newNode.tanOffset;

					const float a = EvalEase(t, kEaseType_SineInOut, 0.f);

				#if 1
					const Vec2F pos = Calc::Lerp(oldPos, newPos, a);
					const Vec2F tan = Calc::Lerp(oldTan, newTan, a);
				#else
					const Vec2F pos = newPos;
					const Vec2F tan = newTan;
				#endif

					nodes[j].m_Position = pos;
					nodes[j].m_Tangent[0] = +tan;
					nodes[j].m_Tangent[1] = -tan;
				}

				BezierPath path;
				path.ConstructFromNodes(nodes, kNumNodes);

				glEnable(GL_LINE_SMOOTH);
				glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);

				gxBegin(GL_LINE_STRIP);
				{
					for (float i = 0.f; i <= kNumNodes; i += 0.01f)
					{
						const Vec2F p = path.Interpolate(i);

						gxVertex2f(p[0], p[1]);
					}
				}
				gxEnd();

				glDisable(GL_LINE_SMOOTH);
			}
		}
		gxPopMatrix();
	}
};

static BezierTest bezierTest;

static void testBezier(float alpha)
{
	bezierTest.tick(framework.timeStep);

	bezierTest.draw(alpha);
}

#endif

#if TEST_FLOWPARTICLES

#define MSX 64
#define MSY 64

#define PS 10000

struct P
{
	float life;
	float lifeRcp;

	float x;
	float y;

	float oldX;
	float oldY;
};

struct M
{
	float v[MSX][MSY][2];

	bool sample(const float x, const float y, float & out_x, float & out_y)
	{
		const float xf = x / GFX_SX * (MSX - 1);
		const float yf = y / GFX_SY * (MSY - 1);

		const int xi = int(xf);
		const int yi = int(yf);

		if (xi < 0 || yi < 0 || xi > MSX - 2 || yi > MSY - 2)
			return false;
		else
		{
			const float dx = xf - xi;
			const float dy = yf - yi;

#if 0
			const float wx1 = 1.f;
			const float wx2 = 0.f;
			
			const float wy1 = 1.f;
			const float wy2 = 0.f;
#else
			const float wx1 = 1.f - dx;
			const float wx2 =       dx;

			const float wy1 = 1.f - dy;
			const float wy2 =       dy;
#endif

			float * out[2] = { &out_x, &out_y };

			for (int i = 0; i < 2; ++i)
			{
				const float vx1 = v[xi + 0][yi + 0][i] * wx1 + v[xi + 1][yi + 0][i] * wx2;
				const float vx2 = v[xi + 0][yi + 1][i] * wx1 + v[xi + 1][yi + 1][i] * wx2;

				*out[i] = vx1 * wy1 + vx2 * wy2;
			}

			return true;
		}
	}
};

static float samplePlasma(const float x, const float y)
{
	const float f1 = std::sin(std::hypot(x, y));

	const float f2 = std::sin(x / (std::cos(y/1.2f) + 1.7f)) * std::cos(y / (sin(x/1.0f) + 1.9f));

	return (f1 + f2) / 2.f + .1f;
	//return f2;
}

static float samplePlasma(const float x, const float y, const float s)
{
	return samplePlasma((x - .5f) * s, (y - .5f) * s);
}

static void sampleWindmap(const float x, const float y, float & out_x, float & out_y)
{
	//const float a = sin(x * 1.234f + framework.time/10.f) * 2.f + sin(y * 2.345f) * 1.6f;
	const float a = x * 2.345f * sin((x - y) * .345f) + y * sin(x * .543f) * 1.234f;
	const float v = (1.1f + sin(cos(x * 2.543) + sin(y * 1.432))) / 2.1f;
	//const float v = 1.f;
	out_x = std::cos(a) * v;
	out_y = std::sin(a) * v;
}

#endif

#if TEST_FIREWORKS

#define PS 20000

enum PT
{
	kPT_Root,
	kPT_Child1,
	kPT_Child2,
};

struct P
{
	float x;
	float y;
	float oldX;
	float oldY;
	float vx;
	float vy;
	float life;
	float lifeRcp;
	int type;
	Color color;
};

#endif

#if TEST_TRACERS

#define GSX (81/1)
#define GSY (81/1)
#define NT 10000
#define TSPEED 200.f

const int directionVectors[4][2] =
{
	{ -1, 0 },
	{ +1, 0 },
	{ 0, -1 },
	{ 0, +1 }
};

struct Tracer
{
	int alive;
	float oldX;
	float oldY;
	float x;
	float y;
	int direction;
	int depth;
	float pixlife;
	float pixlifeRcp;

	bool tick(const float dt)
	{
		oldX = x;
		oldY = y;

		x += directionVectors[direction][0] * TSPEED * dt * 1.f;
		y += directionVectors[direction][1] * TSPEED * dt;

		pixlife -= TSPEED * dt;

		if (pixlife <= 0.f)
			return false;
		else
			return true;
	}
};

struct TraceCell
{
	int value;
};

struct TraceGrid
{
	TraceCell cells[GSX][GSY];

	void draw()
	{
		gxBegin(GL_QUADS);
		{
			for (int x = 0; x < GSX; ++x)
			{
				for (int y = 0; y < GSY; ++y)
				{
					const TraceCell & cell = cells[x][y];

					const float x1 = (x + 0.f) * GFX_SX / GSX;
					const float y1 = (y + 0.f) * GFX_SY / GSY;
					const float x2 = (x + 1.f) * GFX_SX / GSX;
					const float y2 = (y + 1.f) * GFX_SY / GSY;

					if (cell.value % 2)
						continue;
						//setColorf(.2f, .2f, .2f, 1.f);
					else
						setColorf(.5f, .5f, .5f, 1.f);

					drawRect(x1, y1, x2, y2);
				}
			}
		}
		gxEnd();

		if (mouse.isDown(BUTTON_LEFT))
		{
			int x1, y1;
			int x2, y2;

			static int d = 2;

			if (keyboard.wentDown(SDLK_UP))
				d++;
			if (keyboard.wentDown(SDLK_DOWN))
				d--;

			getCellRect(mouse.x, mouse.y, d, x1, y1, x2, y2);

			setColor(colorWhite);

			gxBegin(GL_QUADS);
			{
				for (int x = x1; x < x2; ++x)
				{
					for (int y = y1; y < y2; ++y)
					{
						if (x >= 0 && x < GSX && y >= 0 && y < GSY)
						{
							const TraceCell & cell = cells[x][y];

							const float x1 = (x + 0.f) * GFX_SX / GSX;
							const float y1 = (y + 0.f) * GFX_SY / GSY;
							const float x2 = (x + 1.f) * GFX_SX / GSX;
							const float y2 = (y + 1.f) * GFX_SY / GSY;

							drawRect(x1, y1, x2, y2);
						}
					}
				}
			}
		}
	}

	TraceCell * getCell(const float x, const float y)
	{
		const int cellX = x * GSX / GFX_SX;
		const int cellY = y * GSX / GFX_SY;

		if (cellX >= 0 && cellX < GSX && cellY >= 0 && cellY < GSY)
			return &cells[cellX][cellY];
		else
			return nullptr;
	}

	void getCellRect(const float x, const float y, const int depth, int & x1, int & y1, int & x2, int & y2)
	{
		const int xi = x * GSX / GFX_SX;
		const int yi = y * GSY / GFX_SY;

		x1 = 0;
		y1 = 0;
		x2 = GSX;
		y2 = GSY;

		for (int i = 1; i < depth; ++i)
		{
			const int sx = x2 - x1;
			const int sy = y2 - y1;

			const int xc = (xi - x1) * 3 / (x2 - x1);
			const int yc = (yi - y1) * 3 / (y2 - y1);

			x1 += sx/3 * xc;
			x2 -= sx/3 * (2 - xc);

			y1 += sy/3 * yc;
			y2 -= sy/3 * (2 - yc);
		}
	}
};

struct TraceEffect
{
	TraceGrid grid;

	Tracer tracers[NT];

	float spawnTimer;
	int tracerAllocIndex;

	TraceEffect()
	{
		memset(this, 0, sizeof(*this));
	}

	void tick(const float dt)
	{
		const float kSpawnTime = .5f;
		const float kPixLifePow = 1.f;
		const float kPixLifeMul1 = .9f;
		const float kPixLifeMul2 = .9f;

		spawnTimer += dt;

		while (spawnTimer >= kSpawnTime)
		{
			spawnTimer -= kSpawnTime;

			Tracer & t = tracers[tracerAllocIndex];

			t.alive = 1;
			t.x = GFX_SX/2;
			t.y = GFX_SY/2;
			t.oldX = t.x;
			t.oldY = t.y;
			t.direction = rand() % 4;
			t.depth = 1;
			t.pixlife = 250.f;
			t.pixlifeRcp = 1.f / t.pixlife;

			tracerAllocIndex = (tracerAllocIndex + 1) % NT;
		}

		for (auto & t : tracers)
		{
			if (t.alive)
			{
				if (!t.tick(dt))
				{
					// tracer died. spawn new ones and flip tiles

					int x1, y1;
					int x2, y2;

					grid.getCellRect(t.x, t.y, t.depth + 1, x1, y1, x2, y2);

					x1 = std::max(0, x1);
					y1 = std::max(0, y1);
					x2 = std::min(x2, GSX);
					y2 = std::min(y2, GSY);

					for (int x = x1; x < x2; ++x)
					{
						for (int y = y1; y < y2; ++y)
						{
							TraceCell & cell = grid.cells[x][y];

							cell.value++;
						}
					}

					t.alive = false;

					if (t.depth < 5)
					{
						const float pixlife = 1.f / t.pixlifeRcp;

						Tracer & t1 = tracers[tracerAllocIndex];
						tracerAllocIndex = (tracerAllocIndex + 1) % NT;
						Tracer & t2 = tracers[tracerAllocIndex];
						tracerAllocIndex = (tracerAllocIndex + 1) % NT;

						t1.alive = 1;
						t1.x = t.x;
						t1.y = t.y;
						t1.oldX = t1.x;
						t1.oldY = t1.y;
						//t1.direction = t.direction;
						t1.direction = (t.direction + 1) % 4;
						t1.depth = t.depth + 1;
						t1.pixlife = std::powf(pixlife, kPixLifePow) * random(kPixLifeMul1, kPixLifeMul2);
						t1.pixlifeRcp = 1.f / t1.pixlife;

						t2.alive = 1;
						t2.x = t.x;
						t2.y = t.y;
						t2.oldX = t2.x;
						t2.oldY = t2.y;
						t2.direction = ((t.direction / 2 + 1) * 2 + (rand() % 2)) % 4;
						t2.depth = t.depth + 1;
						t2.pixlife = std::powf(pixlife, kPixLifePow) * random(kPixLifeMul1, kPixLifeMul2);
						t2.pixlifeRcp = 1.f / t2.pixlife;
					}
				}
			}
		}
	}

	void draw()
	{
		grid.draw();

		gxColor4f(1.f, 1.f, 1.f, 1.f);
		glPointSize(4.f);
		gxBegin(GL_LINES);
		{
			for (auto & t : tracers)
			{
				if (t.alive)
				{
					gxVertex2f(t.oldX, t.oldY);
					gxVertex2f(t.x, t.y);
				}
			}
		}
		gxEnd();
	}
};

#endif

int main(int argc, char * argv[])
{
	changeDirectory("data");

	framework.exclusiveFullscreen = false;
	framework.fullscreen = false;
	framework.useClosestDisplayMode = true;

	if (framework.init(0, 0, GFX_SX, GFX_SY))
	{
		//ImageData * image = loadImage("image.png");

		Surface surface(GFX_SX, GFX_SY, true);
		surface.clear();

	#if TEST_FLOWPARTICLES
		M map;
		float moffset[2] = { 0.f, 0.f };

		P ps[PS];
		memset(ps, 0, sizeof(ps));

		float spawnValue = 0.f;
		float spawnValueTreshold = 1.f / 2000.f;

		int nextParticle = 0;
	#endif

	#if TEST_FIREWORKS
		static P ps[PS];
		memset(ps, 0, sizeof(ps));

		int nextParticle = 0;
	#define nextp() ps[nextParticle]; nextParticle = (nextParticle + 1) % PS

		float spawnValue = 0.f;
		float spawnValueTreshold = 1.f / 10.f;
	#endif

	#if TEST_TRACERS
		TraceEffect effect;
	#endif

		while (!framework.quitRequested)
		{
			framework.process();

			if (keyboard.wentDown(SDLK_ESCAPE))
				framework.quitRequested = true;

			framework.beginDraw(0, 0, 0, 0);
			{
		#if TEST_BEZIER
				pushSurface(&surface);
				{
					if (false)
					{
						// don't clear
					}
					else if (true)
					{
						surface.clear();
					}
					else
					{
						setBlend(BLEND_SUBTRACT);
						setColorf(1.f, 1.f, 1.f, 1.f / 200.f);
						drawRect(0, 0, GFX_SX, GFX_SY);
					}

					setBlend(BLEND_ADD);
					testBezier(1.f);
				}
				popSurface();
		#endif

		#if TEST_FLOWPARTICLES
				const float dt = framework.timeStep;

				moffset[0] += dt * .00234f;
				moffset[1] -= dt * .00123f;

				for (int x = 0; x < MSX; ++x)
				{
					for (int y = 0; y < MSY; ++y)
					{
						const float xf = x / (MSX - 1.f) + moffset[0];
						const float yf = y / (MSY - 1.f) + moffset[1];

#if 0
						map.v[x][y][0] = xf;
						map.v[x][y][1] = yf;
#elif 0
						const float e = .001f;
						const float s = 5.f;
						const float m = .08f;

						map.v[x][y][0] = (samplePlasma(xf + e, yf, s) - samplePlasma(xf - e, yf, s)) / (e * 2.f) * m;
						map.v[x][y][1] = (samplePlasma(xf, yf + e, s) - samplePlasma(xf, yf - e, s)) / (e * 2.f) * m;// + .5f;
#else
						//const float s = 4.f + sin(framework.time/10.f) * 10.f;
						const float s = 10.f;
						const float m = .5f;

						const float sx = (xf - .5f + moffset[0]) * s;
						const float sy = (yf - .5f + moffset[1]) * s;

						sampleWindmap(sx, sy, map.v[x][y][0], map.v[x][y][1]);

						map.v[x][y][0] *= m;
						map.v[x][y][1] *= m;
#endif
					}
				}

				spawnValue += dt;

				while (spawnValue >= spawnValueTreshold)
				{
					spawnValue -= spawnValueTreshold;

					P & p = ps[nextParticle];

					p.x = random(0.f, float(GFX_SX));
					p.y = random(0.f, float(GFX_SY));
					p.oldX = p.x;
					p.oldY = p.y;
					p.life = PS * spawnValueTreshold;
					p.lifeRcp = 1.f / p.life;

					nextParticle = (nextParticle + 1) % PS;
				}

				const float speedMul = dt * 200.f;

				const Color baseColor = Color::fromHSL(framework.time / 10.f, .1f, .5f);
				//const Color baseColor(1.f, .5f, .25f);

				for (int i = 0; i < PS; ++i)
				{
					P & p = ps[i];

					if (p.life > 0.f)
					{
						p.life -= dt;

						float v[2];

						if (map.sample(p.x, p.y, v[0], v[1]))
						{
							p.oldX = p.x;
							p.oldY = p.y;

							p.x += v[0] * speedMul;
							p.y += v[1] * speedMul;
						}
					}
				}

				//

				pushSurface(&surface);
				{
					const float c = keyboard.isDown(SDLK_SPACE) ? 0.f : 0.99f;//1.f / 150.f;
					setBlend(BLEND_MUL);
					setColorf(c, c, c, c);
					drawRect(0, 0, GFX_SX, GFX_SY);

					setBlend(BLEND_ADD);
					//setBlend(BLEND_ALPHA);

					glPointSize(2.f);
					//glLineWidth(2.f);

					glEnable(GL_LINE_SMOOTH);
					glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);

					gxBegin(GL_LINES);
					//gxBegin(GL_POINTS);
					{
	#if 1
						for (int i = 0; i < PS; ++i)
						{
							const P & p = ps[i];

							if (p.life > 0.f)
							{
								float v[2];

								if (map.sample(p.x, p.y, v[0], v[1]))
								{
									const float s = (std::sqrt(v[0] * v[0] + v[1] * v[1]) + .3f) * 1.f;
									//const float s = .1f;

									gxColor4f(baseColor.r * s, baseColor.g * s, baseColor.b * s, p.life * p.lifeRcp * 1.f);
									gxVertex2f(p.oldX, p.oldY);
									gxVertex2f(p.x, p.y);
								}
							}
						}
	#endif

	#if 0
						for (int i = 0; i < 10000; ++i)
						{
							const float t = i / 10000.f;

							const float x = GFX_SX/2 + std::cos(t * Calc::m2PI * 100) * t * 800;
							const float y = GFX_SY/2 + std::sin(t * Calc::m2PI * 100) * t * 500;

							float v[2];

							if (map.sample(x, y, v[0], v[1]))
							{
								gxColor4f((v[0] + 1.f) / 2.f, (v[1] + 1.f) / 2.f, 0.5f, 1.f);
								gxVertex2f(x, y);
							}
							else
							{
								gxColor4f(1.f, 1.f, 1.f, 1.f);
								gxVertex2f(x, y);
							}
						}
	#endif

						glDisable(GL_LINE_SMOOTH);
					}
					gxEnd();
				}
				popSurface();
			#endif

#if TEST_FIREWORKS
				static int mode = 0;

				if (keyboard.wentDown(SDLK_m))
					mode = (mode + 1) % 2;

				const float dt = framework.timeStep;

				spawnValue += dt;

				while (spawnValue >= spawnValueTreshold)
				{
					spawnValue -= spawnValueTreshold;

					P & p = nextp();

					const float h = pow(random(0.f, 1.f), .5f);
					const float r = Calc::DegToRad(35);
					const float a = Calc::m2PI*3/4 + random(-r/2, +r/2);
					const float v = lerp(200.f, 900.f, h);
					
					p.type = kPT_Root;
					p.x = GFX_SX/2.f + random(-100.f, +100.f);
					//p.y = random(0.f, float(GFX_SY));
					p.y = GFX_SY;
					p.oldX = p.x;
					p.oldY = p.y;
					p.life = 1.f;
					p.lifeRcp = 1.f / p.life;
					p.vx = std::cos(a) * v;
					p.vy = std::sin(a) * v;
					p.color = Color::fromHSL(.5f + random(0.f, .3f), .5f, .5f);
				}

				for (int i = 0; i < PS; ++i)
				{
					P & p = ps[i];

					if (p.life > 0.f)
					{
						p.life -= dt;

						if (mode == 0)
						{
							p.oldX = p.x;
							p.oldY = p.y;
						}

						p.x += p.vx * dt;
						p.y += p.vy * dt;

						p.vy += 40.f * dt;

						if (p.life <= 0.f)
						{
							if (p.type == kPT_Root)
							{
								for (int i = 0; i < 200; ++i)
								{
									P & pc = nextp();

									const float a = random(0.f, Calc::m2PI);
									const float v = random(80.f, 100.f);

									const float pv = .05f;

									pc.type = kPT_Child1;
									pc.x = p.x;
									pc.y = p.y;
									pc.oldX = pc.x;
									pc.oldY = pc.y;
									pc.life = 2.f;
									pc.lifeRcp = 1.f / pc.life;
									pc.vx = std::cos(a) * v + p.vx * pv;
									pc.vy = std::sin(a) * v + p.vy * pv;
									pc.color = p.color;

									P & pm = nextp();
									pm = pc;
									pm.vx *= -1.f;
								}
							}

							if (p.type == kPT_Child1)
							{
								if ((rand() % 20) == 0)
								{
									for (int i = 0; i < 10; ++i)
									{
										P & pc = nextp();

										const float a = random(0.f, Calc::m2PI);
										const float v = random(20.f, 30.f);

										const float pv = 0.f;

										pc.type = kPT_Child2;
										pc.x = p.x;
										pc.y = p.y;
										pc.oldX = pc.x;
										pc.oldY = pc.y;
										pc.life = 1.f;
										pc.lifeRcp = 1.f / pc.life;
										pc.vx = std::cos(a) * v;
										pc.vy = std::sin(a) * v - 10.f;
										pc.color = p.color;

										P & pm = nextp();
										pm = pc;
										pm.vx *= -1.f;
									}
								}
							}
						}
					}
				}

				pushSurface(&surface);
				{
					if (mode == 0)
					{
						const float c = .99f;
						//const float c = 0.f;
						setBlend(BLEND_MUL);
						setColorf(c, c, c, c);
						drawRect(0, 0, GFX_SX, GFX_SY);
					}
					if (mode == 1)
					{
						surface.clear();
					}

					//

					glEnable(GL_LINE_SMOOTH);
					glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);

					setBlend(BLEND_ADD);

					glPointSize(4.f);

					gxBegin(GL_LINES);
					{
						for (int i = 0; i < PS; ++i)
						{
							const P & p = ps[i];

							if (p.life != 0.f)
							{
								gxColor4f(p.color.r, p.color.g, p.color.b, p.life * p.lifeRcp * 2.f);
								//gxColor4f(p.color.r, p.color.g, p.color.b, 1.f);
								
								if (mode == 0)
								{
									gxVertex2f(p.oldX, p.oldY);
									gxVertex2f(p.x,    p.y);
								}
								if (mode == 1)
								{
									//gxVertex2f(p.oldX, p.oldY);
									//gxVertex2f(p.x,    p.oldY);

									//gxVertex2f(p.oldX, p.oldY);
									//gxVertex2f(p.oldX, p.y);

									//gxVertex2f(p.oldX, p.y);
									//gxVertex2f(p.x,    p.y);

									//gxVertex2f(p.x, p.oldY);
									//gxVertex2f(p.x, p.y);

									gxVertex2f(p.oldX, p.oldY);
									gxVertex2f(p.x,    p.oldY);
									gxVertex2f(p.x,    p.oldY);
									gxVertex2f(p.x,    p.y   );
									gxVertex2f(p.x,    p.y   );
									gxVertex2f(p.oldX, p.y   );
									gxVertex2f(p.oldX, p.y   );
									gxVertex2f(p.oldX, p.oldY);
								}
							}
						}
					}
					gxEnd();

					glDisable(GL_LINE_SMOOTH);
				}
				popSurface();
#endif

#if TEST_TRACERS
				const float dt = framework.timeStep;

				effect.tick(dt);

				pushSurface(&surface);
				{
					const float c = .95f;
					//const float c = 0.f;
					setBlend(BLEND_MUL);
					setColorf(c, c, c, c);
					drawRect(0, 0, GFX_SX, GFX_SY);
					setBlend(BLEND_ALPHA);

					//surface.clear();

					effect.draw();
				}
				popSurface();
#endif

				setBlend(BLEND_OPAQUE);

				gxSetTexture(surface.getTexture());
				{
					gxColor4f(1.f, 1.f, 1.f, 1.f);
					drawRect(0, 0, GFX_SX, GFX_SY);
				}
				gxSetTexture(0);
			}
			framework.endDraw();
		}

		framework.shutdown();
	}

	return 0;
}
