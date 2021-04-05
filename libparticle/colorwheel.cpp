#include "colorwheel.h"
#include "framework.h"
#include "ui.h"
#include <algorithm>

#if defined(DEBUG)
	#define DO_DEBUG_TEXT 0
#else
	#define DO_DEBUG_TEXT 0 // do not alter
#endif

void ColorWheel::getTriangleAngles(float a[3])
{
	a[0] = 2.f * float(M_PI) / 3.f * 0.f;
	a[1] = 2.f * float(M_PI) / 3.f * 1.f;
	a[2] = 2.f * float(M_PI) / 3.f * 2.f;
}

void ColorWheel::getTriangleCoords(float xy[6])
{
	const float r = size - wheelThickness;

	float a[3];
	getTriangleAngles(a);

	for (int i = 0; i < 3; ++i)
	{
		xy[i * 2 + 0] = cosf(a[i]) * r;
		xy[i * 2 + 1] = sinf(a[i]) * r;
	}
}

ColorWheel::ColorWheel()
	: hue(0.f)
	, baryWhite(0.f)
	, baryBlack(0.f)
	, opacity(1.f)
	, state(kState_Idle)
{
}

void ColorWheel::tick(const float _mouseX, const float _mouseY, const bool mouseWentDown, const bool mouseIsDown, const float dt)
{
	const float mouseX = _mouseX - size;
	const float mouseY = _mouseY - size;

	switch (state)
	{
	case kState_Idle:
		{
			float h, bw, bb;
			float t;

			if (mouseWentDown && intersectWheel(mouseX, mouseY, h))
			{
				state = kState_SelectHue;

				hue = h;
			}
			else if (mouseWentDown && intersectTriangle(mouseX, mouseY, bw, bb))
			{
				state = kState_SelectSatValue;

				baryWhite = bw;
				baryBlack = bb;
			}
			else if (mouseWentDown && intersectBar(mouseX, mouseY, t))
			{
				state = kState_SelectOpacity;
			}
		}
		break;

	case kState_SelectHue:
		{
			if (mouseIsDown)
				intersectWheel(mouseX, mouseY, hue);
			else
				state = kState_Idle;
		}
		break;

	case kState_SelectSatValue:
		{
			if (mouseIsDown)
				intersectTriangle(mouseX, mouseY, baryWhite, baryBlack);
			else
				state = kState_Idle;
		}
		break;

	case kState_SelectOpacity:
		{
			if (mouseIsDown)
				intersectBar(mouseX, mouseY, opacity);
			else
				state = kState_Idle;
		}
	}
}

void ColorWheel::draw()
{
	gxPushMatrix();
	gxTranslatef(size, size, 0.f);

	const int numSteps = 200;
	const float r1 = size;
	const float r2 = size - wheelThickness;

	// color wheel

	gxBegin(GX_QUADS);
	{
		for (int i = 0; i < numSteps; ++i)
		{
			float r, g, b, a;
			toColor(i / float(numSteps) * 2.f * M_PI, 0.f, 0.f, r, g, b, a);
			gxColor4f(r, g, b, 1.f);

			const float a1 = (i + 0) / float(numSteps) * 2.f * M_PI;
			const float a2 = (i + 1) / float(numSteps) * 2.f * M_PI;
			const float x1 = cosf(a1);
			const float y1 = sinf(a1);
			const float x2 = cosf(a2);
			const float y2 = sinf(a2);

			gxVertex2f(x1 * r1, y1 * r1);
			gxVertex2f(x2 * r1, y2 * r1);
			gxVertex2f(x2 * r2, y2 * r2);
			gxVertex2f(x1 * r2, y1 * r2);
		}
	}
	gxEnd();

	// selection circle on the color wheel
	
	for (int i = 0; i < 3; ++i)
	{
		const float c = i == 0 ? 1.f : 0.5f;
		const float a = hue + i / 3.f * 2.f * float(M_PI);
		const float x = cosf(a) * (size - wheelThickness / 2.f);
		const float y = sinf(a) * (size - wheelThickness / 2.f);
		drawUiCircle(x, y, 5.5f, c, c, c, 1.f);
	}

	// lightness triangle

	{
		float xy[6];
		getTriangleCoords(xy);
		
		float r, g, b, a;
		toColor(hue, 0.f, 0.f, r, g, b, a);
		
		const Color c1(r, g, b, 1.f);
		const Color c2(0.f, 0.f, 0.f, 1.f);
		const Color c3(1.f, 1.f, 1.f, 1.f);
		
		setColor(colorWhite);
		drawUiShadedTriangle(xy[0], xy[1], xy[2], xy[3], xy[4], xy[5], c1, c2, c3);
	}

	{
		float xy[6];
		getTriangleCoords(xy);

		setColor(colorWhite);
		const float x = xy[0] + (xy[2] - xy[0]) * baryBlack + (xy[4] - xy[0]) * baryWhite;
		const float y = xy[1] + (xy[3] - xy[1]) * baryBlack + (xy[5] - xy[1]) * baryWhite;
		drawUiCircle(x, y, 5.5f, 1.f, 1.f, 1.f, 1.f);
	}

	{
		setColor(colorWhite);
		drawUiRectCheckered(barX1, barY1, barX2, barY2, barHeight / 2.f);
		float r, g, b, a;
		toColor(r, g, b, a);
		setColorf(r, g, b, a);
		drawRect(barX1, barY1, barX2, barY2);
		setColor(colorBlue);
		drawRectLine(barX1, barY1, barX2, barY2);
		
		setColor(colorWhite);
		const float x = barX1 * (1.f - a) + barX2 * a;
		const float y = (barY1 + barY2) / 2.f;
		drawUiCircle(x, y, 3.5f, 1.f, 1.f, 1.f, 1.f);
	}

#if DO_DEBUG_TEXT
	if (g_doDraw)
	{
		float r, g, b, a;
		toColor(r, g, b, a);
		setColorf(r, g, b, a);
		drawRect(0.f, 0.f, 100.f, 20.f);
		setColorf(1.f, 1.f, 1.f, 1.f);
		drawText(0.f, 0.f, 12, +1.f, +1.f, "%f, %f, %f", r, g, b);
	}
#endif

	gxPopMatrix();
}

bool ColorWheel::intersectWheel(const float x, const float y, float & hue) const
{
	const float dSq = x * x + y * y;
	if (dSq != 0.f)
		hue = atan2f(y, x);
	if (dSq > size * size)
		return false;
	if (dSq < (size - wheelThickness) * (size - wheelThickness))
		return false;
	return true;
}

static void computeBarycentric(const float x1, const float y1, const float x2, const float y2, const float x3, const float y3, const float x, const float y, float & b1, float & b2, float & b3)
{
	b1 = ((y2 - y3) * (x - x3) + (x3 - x2) * (y - y3)) / ((y2 - y3) * (x1 - x3) + (x3 - x2) * (y1 - y3));
	b2 = ((y3 - y1) * (x - x3) + (x1 - x3) * (y - y3)) / ((y2 - y3) * (x1 - x3) + (x3 - x2) * (y1 - y3));
	b3 = 1.f - b1 - b2;
}

bool ColorWheel::intersectTriangle(const float x, const float y, float & baryWhite, float & baryBlack) const
{
	float xy[6];
	getTriangleCoords(xy);

	float temp;
	computeBarycentric(
		xy[0], xy[1],
		xy[2], xy[3],
		xy[4], xy[5],
		x, y, temp, baryBlack, baryWhite);

	const bool inside =
		baryWhite >= 0.f && baryWhite <= 1.f &&
		baryBlack >= 0.f && baryBlack <= 1.f;

	baryWhite = std::max(0.f, baryWhite);
	baryBlack = std::max(0.f, baryBlack);

	const float bs = baryWhite + baryBlack;
	if (bs > 1.f)
	{
		baryWhite /= bs;
		baryBlack /= bs;
	}

#if DO_DEBUG_TEXT
	if (g_doDraw)
	{
		setColor(colorWhite);
		drawText(xy[2], xy[3], 16, +1.f, +1.f, "bb: %f", baryWhite);
		drawText(xy[4], xy[5], 16, +1.f, +1.f, "bw: %f", baryBlack);
	}
#endif

	return inside;
}

bool ColorWheel::intersectBar(const float x, const float y, float & t) const
{
	t = saturate((x - barX1) / (barX2 - barX1));
	return x >= barX1 && x <= barX2 && y >= barY1 && y <= barY2;
}

void ColorWheel::fromColor(const float r, const float g, const float b, const float a)
{
	float lum, sat;
	rgbToHSL(r, g, b, hue, lum, sat);

	hue = hue * 2.f * float(M_PI);
	baryWhite = lum > .5f ? (lum - .5f) * 2.f : 0.f;
	baryBlack = lum < .5f ? (.5f - lum) * 2.f : 0.f;

	// bw + bb + t * 2 = 1
	// t = (1 - bw - bb) / 2
	const float t = (1.f - baryWhite - baryBlack) / 2.f;
	const float s = 1.f - sat;
	baryWhite += t * s;
	baryBlack += t * s;

	opacity = a;
}

void ColorWheel::toColor(const float hue, const float baryWhite, const float baryBlack, float & r, float & g, float & b, float & a) const
{
	const float baryColor = 1.f - baryWhite - baryBlack;

	hlsToRGB(hue / (2.f * float(M_PI)), .5f, 1.f, r, g, b);

	r = r * baryColor + 1.f * baryWhite + 0.f * baryBlack;
	g = g * baryColor + 1.f * baryWhite + 0.f * baryBlack;
	b = b * baryColor + 1.f * baryWhite + 0.f * baryBlack;
	
	a = opacity;
}

void ColorWheel::toColor(float & r, float & g, float & b, float & a) const
{
	toColor(hue, baryWhite, baryBlack, r, g, b, a);
}
