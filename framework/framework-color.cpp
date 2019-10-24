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
#include "StringEx.h"
#include <algorithm>

static const float rcp255 = 1.f / 255.f;

static float scale255(const float v)
{
	return v * rcp255;
}

Color::Color()
{
	r = g = b = a = 0.f;
}

Color::Color(int r, int g, int b, int a)
{
	this->r = scale255(r);
	this->g = scale255(g);
	this->b = scale255(b);
	this->a = scale255(a);
}

Color::Color(float r, float g, float b, float a)
{
	this->r = r;
	this->g = g;
	this->b = b;
	this->a = a;
}

Color Color::fromHex(const char * str)
{
	if (str[0] == '#')
		str++;
	
	const size_t len = strlen(str);
	
	if (len == 0)
	{
		return Color(0.f, 0.f, 0.f, 0.f);
	}
	else if (len == 3)
	{
		const uint32_t hex = std::stoul(str, 0, 16);
		const float r = scale255(((hex >> 8) & 0xf) * 255/15);
		const float g = scale255(((hex >> 4) & 0xf) * 255/15);
		const float b = scale255(((hex >> 0) & 0xf) * 255/15);
		const float a = 1.f;
		return Color(r, g, b, a);
	}
	else if (len == 4)
	{
		const uint32_t hex = std::stoul(str, 0, 16);
		const float r = scale255(((hex >> 12) & 0xf) * 255/15);
		const float g = scale255(((hex >>  8) & 0xf) * 255/15);
		const float b = scale255(((hex >>  4) & 0xf) * 255/15);
		const float a = scale255(((hex >>  0) & 0xf) * 255/15);
		return Color(r, g, b, a);
	}
	else if (len == 6)
	{
		const uint32_t hex = std::stoul(str, 0, 16);
		const float r = scale255((hex >> 16) & 0xff);
		const float g = scale255((hex >>  8) & 0xff);
		const float b = scale255((hex >>  0) & 0xff);
		const float a = 1.f;
		return Color(r, g, b, a);
	}
	else if (len == 8)
	{
		const uint32_t hex = std::stoul(str, 0, 16);
		const float r = scale255((hex >> 24) & 0xff);
		const float g = scale255((hex >> 16) & 0xff);
		const float b = scale255((hex >>  8) & 0xff);
		const float a = scale255((hex >>  0) & 0xff);
		return Color(r, g, b, a);
	}
	else
	{
		return colorBlack;
	}
}

Color Color::fromHSL(float hue, float sat, float lum)
{
	hue = fmod(hue, 1.f) * 6.f;
	sat = sat < 0.f ? 0.f : sat > 1.f ? 1.f : sat;
	lum = lum < 0.f ? 0.f : lum > 1.f ? 1.f : lum;
	
	//
	
	float r, g, b;

	float m2 = (lum <= .5f) ? (lum + (lum * sat)) : (lum + sat - lum * sat);
	float m1 = lum + lum - m2;

	if (hue < 0.f)
	{
		hue += 6.f;
	}

	if (hue < 3.0f)
	{
		if (hue < 2.0f)
		{
			if (hue < 1.0f)
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

	return Color(r, g, b);
}

void Color::toHSL(float & hue, float & sat, float & lum) const
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

Color Color::interp(const Color & other, float t) const
{
	const float t1 = 1.f - t;
	const float t2 = t;
	
	return Color(
		r * t1 + other.r * t2,
		g * t1 + other.g * t2,
		b * t1 + other.b * t2,
		a * t1 + other.a * t2);
}

Color Color::hueShift(float shift) const
{
	float hue, sat, lum;
	toHSL(hue, sat, lum);
	return Color::fromHSL(hue + shift, sat, lum);
}

uint32_t Color::toRGBA() const
{
	const int ir = r < 0.f ? 0 : r > 1.f ? 255 : int(r * 255.f);
	const int ig = g < 0.f ? 0 : g > 1.f ? 255 : int(g * 255.f);
	const int ib = b < 0.f ? 0 : b > 1.f ? 255 : int(b * 255.f);
	const int ia = a < 0.f ? 0 : a > 1.f ? 255 : int(a * 255.f);
	return (ir << 24) | (ig << 16) | (ib << 8) | (ia << 0);
}

std::string Color::toHexString(const bool withAlpha) const
{
	const int ir = r < 0.f ? 0 : r > 1.f ? 255 : int(r * 255.f);
	const int ig = g < 0.f ? 0 : g > 1.f ? 255 : int(g * 255.f);
	const int ib = b < 0.f ? 0 : b > 1.f ? 255 : int(b * 255.f);
	const int ia = a < 0.f ? 0 : a > 1.f ? 255 : int(a * 255.f);
	
	char text[64];
	
	if (withAlpha)
		sprintf_s(text, sizeof(text), "%02x%02x%02x%02x", ir, ig, ib, ia);
	else
		sprintf_s(text, sizeof(text), "%02x%02x%02x", ir, ig, ib);
	
	return text;
}

void Color::set(const float r, const float g, const float b, const float a)
{
	this->r = r;
	this->g = g;
	this->b = b;
	this->a = a;
}

Color Color::addRGB(const Color & other) const
{
	return Color(r + other.r, g + other.g, b + other.b, a);
}

Color Color::mulRGBA(const Color & other) const
{
	return Color(r * other.r, g * other.g, b * other.b, a * other.a);
}

Color Color::mulRGB(float t) const
{
	return Color(r * t, g * t, b * t, a);
}
