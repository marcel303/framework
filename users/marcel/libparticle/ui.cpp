#include "framework.h"
#include "ui.h"

void drawRectCheckered(float x1, float y1, float x2, float y2, float scale)
{
	static GLuint checkersTexture = 0;
	// fixme : move to init!
	if (checkersTexture == 0)
	{
		const uint8_t v1 = 31;
		const uint8_t v2 = 63;
		uint32_t rgba[4];
		uint32_t c1; uint32_t c2;
		uint8_t * rgba1 = (uint8_t*)&c1; uint8_t * rgba2 = (uint8_t*)&c2;
		rgba1[0] = v1; rgba1[1] = v1; rgba1[2] = v1; rgba1[3] = 255;
		rgba2[0] = v2; rgba2[1] = v2; rgba2[2] = v2; rgba2[3] = 255;
		rgba[0] = c1; rgba[1] = c2; rgba[2] = c2; rgba[3] = c1;
		checkersTexture = createTextureFromRGBA8(rgba, 2, 2, false, false);
	}
	
	gxSetTexture(checkersTexture);
	
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	gxBegin(GL_QUADS);
	{
		gxTexCoord2f(0.f,               (y2 - y1) / scale); gxVertex2f(x1, y1);
		gxTexCoord2f((x2 - x1) / scale, (y2 - y1) / scale); gxVertex2f(x2, y1);
		gxTexCoord2f((x2 - x1) / scale, 0.f              ); gxVertex2f(x2, y2);
		gxTexCoord2f(0.f,               0.f              ); gxVertex2f(x1, y2);
	}
	gxEnd();
	gxSetTexture(0);
}

void drawUiCircle(const float x, const float y, const float radius, const float r, const float g, const float b, const float a)
{
	hqBegin(HQ_STROKED_CIRCLES);
	{
		setColorf(0.f, 0.f, 0.f, a);
		hqStrokeCircle(x, y, radius, 4.f);
		setColorf(r, g, b, a);
		hqStrokeCircle(x, y, radius, 3.f);
	}
	hqEnd();
}

void hlsToRGB(float hue, float lum, float sat, float & r, float & g, float & b)
{
	float m2 = (lum <= .5f) ? (lum + (lum * sat)) : (lum + sat - lum * sat);
	float m1 = lum + lum - m2;

	hue = fmod(hue, 1.f) * 6.f;

	if (hue < 0.f)
	{
		hue += 6.f;
	}

	if (hue < 3.0f)
	{
		if (hue < 2.0f)
		{
			if(hue < 1.0f)
			{
				r = m2;
				g = m1 + (m2 - m1) * hue;
				b = m1;
			}
			else
			{
				r = (m1 + (m2 - m1) * (2.f - hue));
				g = m2;
				b = m1;
			}
		}
		else
		{
			r = m1;
			g = m2;
			b = (m1 + (m2 - m1) * (hue - 2.f));
		}
	}
	else
	{
		if (hue < 5.0f)
		{
			if (hue < 4.0f)
			{
				r = m1;
				g = (m1 + (m2 - m1) * (4.f - hue));
				b = m2;
			}
			else
			{
				r = (m1 + (m2 - m1) * (hue - 4.f));
				g = m1;
				b = m2;
			}
		}
		else
		{
			r = m2;
			g = m1;
			b = (m1 + (m2 - m1) * (6.f - hue));
		}
	}
}

void rgbToHSL(float r, float g, float b, float & hue, float & lum, float & sat)
{
	float max = std::max(r, std::max(g, b));
	float min = std::min(r, std::min(g, b));

	lum = (max + min) / 2.0f;

	float delta = max - min;

	if (delta < FLT_EPSILON)
	{
		sat = 0.f;
		hue = 0.f;
	}
	else
	{
		sat = (lum <= .5f) ? (delta / (max + min)) : (delta / (2.f - (max + min)));

		if (r == max)
			hue = (g - b) / delta;
		else if (g == max)
			hue = 2.f + (b - r) / delta;
		else
			hue = 4.f + (r - g) / delta;

		if (hue < 0.f)
			hue += 6.0f;

		hue /= 6.f;
	}
}
