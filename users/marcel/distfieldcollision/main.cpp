#include "Calc.h"
#include "framework.h"
#include "image.h"
#include <vector>

struct SDF
{
	static const int kSize = 64;

	float value[kSize][kSize];

	void clear()
	{
		for (int x = 0; x < SDF::kSize; ++x)
		{
			for (int y = 0; y < SDF::kSize; ++y)
			{
				value[y][x] = std::numeric_limits<float>::max();
			}
		}
	}

	void calculateAt(int sx, int sy, int tx, int ty)
	{
		for (int ox = -1; ox <= +1; ++ox)
		{
			for (int oy = -1; oy <= +1; ++oy)
			{
				if (ox == 0 && oy == 0)
					continue;

				const int x = tx + ox;
				const int y = ty + oy;

				if (x < 0 || y < 0 || x >= kSize || y >= kSize)
					continue;

				const int dx = x - sx;
				const int dy = y - sy;

				const int dSquared = dx * dx + dy * dy;

				if (dSquared < value[y][x])
				{
					value[y][x] = dSquared;

					calculateAt(sx, sy, x, y);
				}
			}
		}
	}

	void calculate()
	{
		for (int x = 0; x < SDF::kSize; ++x)
		{
			for (int y = 0; y < SDF::kSize; ++y)
			{
				if (value[y][x] == 0)
				{
					calculateAt(x, y, x, y);
				}
			}
		}

		for (int x = 0; x < SDF::kSize; ++x)
		{
			for (int y = 0; y < SDF::kSize; ++y)
			{
				value[y][x] = std::sqrtf(value[y][x]);
			}
		}
	}
};

static bool createSignedDistanceField(SDF & sdf, GLuint & texture)
{
	sdf.clear();

	for (int i = 0; i < 10; ++i)
	{
		const int x = rand() % SDF::kSize;
		const int y = rand() % SDF::kSize;

		sdf.value[y][x] = 0;
	}

	sdf.calculate();

	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glTexImage2D(
		GL_TEXTURE_2D,
		0,
		GL_R32F,
		sdf.kSize,
		sdf.kSize,
		0,
		GL_RED,
		GL_FLOAT,
		sdf.value);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	if (texture == 0)
	{
		return false;
	}
	else
	{
		return true;
	}
}

static void drawCircleLine(float x, float y, float radius, int numSegments)
{
	gxBegin(GL_LINE_LOOP);
	{
		for (int i = 0; i < numSegments; ++i)
		{
			const float a = (i + 0) / float(numSegments) * 2.f * M_PI;

			gxVertex2f(x + std::sinf(a) * radius, y + std::cosf(a) * radius);
		}
	}
	gxEnd();
}

int main(int argc, char * argv[])
{
	const int displaySize = 256;
	const int displayScale = displaySize / SDF::kSize;
	const int collisionRadius = 25;

	if (framework.init(0, 0, displaySize, displaySize))
	{
		SDF sdf;
		GLuint texture;

		createSignedDistanceField(sdf, texture);

		struct Point
		{
			int x, y;
		};

		std::vector<Point> points;

		for (int x = 0; x < SDF::kSize; ++x)
		{
			for (int y = 0; y < SDF::kSize; ++y)
			{
				if (sdf.value[y][x] == 0.f)
				{
					Point p;
					p.x = x;
					p.y = y;
					points.push_back(p);
				}
			}
		}

		float scale = 1.f;

		bool stop = false;

		while (!stop)
		{
			framework.process();

			if (keyboard.isDown(SDLK_ESCAPE))
				stop = true;

			const int x = Calc::Clamp(mouse.x / displayScale, 0, SDF::kSize - 1);
			const int y = Calc::Clamp(mouse.y / displayScale, 0, SDF::kSize - 1);
			const float distance = sdf.value[y][x] * displayScale;

			if (mouse.isDown(BUTTON_RIGHT))
				scale = mouse.y / float(displaySize);

			framework.beginDraw(0, 0, 0, 0);
			{
				setBlend(BLEND_ALPHA);
				gxSetTexture(texture);
				setColorf(scale, scale, scale);
				gxBegin(GL_QUADS);
				{
					gxTexCoord2f(0.f, 0.f); gxVertex2f(0, 0);
					gxTexCoord2f(1.f, 0.f); gxVertex2f(256, 0);
					gxTexCoord2f(1.f, 1.f); gxVertex2f(256, 256);
					gxTexCoord2f(0.f, 1.f); gxVertex2f(0, 256);
				}
				gxEnd();
				gxSetTexture(0);

				//

				for (auto & p : points)
				{
					setColor(colorBlue);
					drawRect(
						p.x * displayScale - 1,
						p.y * displayScale - 1,
						p.x * displayScale + 1,
						p.y * displayScale + 1);
				}

				//

				setColor(distance <= collisionRadius ? colorYellow : colorGreen);
				drawCircleLine(mouse.x, mouse.y, collisionRadius, 16);

				setFont("calibri.ttf");
				drawText(mouse.x, mouse.y + 24, 16, 0, 0, "%02.2f", distance);
			}
			framework.endDraw();
		}

		framework.shutdown();
	}

	return 0;
}
