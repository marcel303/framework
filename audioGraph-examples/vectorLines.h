#pragma once

#include "Calc.h"
#include "framework.h"
#include <cmath>

struct VectorParticleSystem
{
	static const int kMaxParticles = 220;

	float lt[kMaxParticles];
	float lr[kMaxParticles];
	float x[kMaxParticles];
	float y[kMaxParticles];
	float z[kMaxParticles];
	float vx[kMaxParticles];
	float vy[kMaxParticles];
	float vz[kMaxParticles];

	double spawnTimer;
	double spawnInterval;
	int spawnIndex;

	VectorParticleSystem()
	{
		memset(this, 0, sizeof(*this));
	}

	void spawn(const float life)
	{
		const float x1 = -100.f;
		const float y1 = -1.f;
		const float z1 = -1.f;
		const float x2 = +0.f;
		const float y2 = +1.f;
		const float z2 = +1.f;

		lt[spawnIndex] = life;
		lr[spawnIndex] = 1.f / life;
		x[spawnIndex] = random(x1, x2);
		y[spawnIndex] = random(y1, y2);
		z[spawnIndex] = random(z1, z2);
		vx[spawnIndex] = random(-1.f, +1.f);
		vy[spawnIndex] = random(-1.f, +2.f);
		vz[spawnIndex] = random(-1.f, +1.f);

		spawnIndex = (spawnIndex + 1) % kMaxParticles;
	}

	void tick(const float dt)
	{
		spawnInterval = 2.0 / 60.0;

		if (spawnInterval > 0.0)
		{
			spawnTimer -= dt;

			while (spawnTimer <= 0.0)
			{
				spawnTimer += spawnInterval;

				const double life = kMaxParticles * spawnInterval;

				spawn(life);
			}
		}
		else
		{
			spawnTimer = 0.0;
		}

		for (int i = 0; i < kMaxParticles; ++i)
		{
			lt[i] -= dt;

			x[i] += vx[i] * dt;
			y[i] += vy[i] * dt;
			z[i] += vz[i] * dt;

			vy[i] += -.5f * dt;
		}
	}

	void draw() const
	{
		const float kMaxSize = 4.f;

		glEnable(GL_PROGRAM_POINT_SIZE);
		checkErrorGL();
		
		gxBegin(GL_POINTS);
		{
			for (int i = 0; i < kMaxParticles; ++i)
			{
				if (lt[i] > 0.f)
				{
					const float l = lt[i] * lr[i];

					const float size = std::sin(l * Calc::mPI) * ((6.f - std::cos(l * Calc::mPI * 7.f)) / 5.f) * kMaxSize;

					gxTexCoord2f(size, 0.f);
					gxVertex3f(x[i], y[i], z[i]);
				}
			}
		}
		gxEnd();
		
		glDisable(GL_PROGRAM_POINT_SIZE);
		checkErrorGL();
	}
};

struct VectorMemory
{
	const static int kMaxLines = 100000;

	struct Line
	{
		float lt, lr;
		float x1, y1, z1;
		float x2, y2, z2;
	};

	Line lines[kMaxLines];

	int lineAllocIndex;

	VectorMemory()
	{
		memset(this, 0, sizeof(*this));
	}

	void addLine(
		const float x1,
		const float y1,
		const float z1,
		const float x2,
		const float y2,
		const float z2,
		const float life)
	{
		Line & line = lines[lineAllocIndex];

		lineAllocIndex = (lineAllocIndex + 1) % kMaxLines;

		line.lt = life;
		line.lr = 1.f / life;

		line.x1 = x1;
		line.y1 = y1;
		line.z1 = z1;
		line.x2 = x2;
		line.y2 = y2;
		line.z2 = z2;
	}

	void tick(const float dt)
	{
		for (int i = 0; i < kMaxLines; ++i)
		{
			lines[i].lt -= dt;
		}
	}

	void draw(const float opacity) const
	{
		gxBegin(GL_LINES);
		{
			for (int i = 0; i < kMaxLines; ++i)
			{
				const Line & line = lines[i];

				if (line.lt > 0.f)
				{
					const float l = line.lt * line.lr * opacity;

					gxColor4f(l, l, l, l);

					gxVertex3f(line.x1, line.y1, line.z1);
					gxVertex3f(line.x2, line.y2, line.z2);
				}
			}
		}
		gxEnd();
	}
};
