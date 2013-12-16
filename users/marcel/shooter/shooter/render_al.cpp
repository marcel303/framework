#include <allegro.h>
#include <math.h>
#include <stdio.h>
#include "input.h"
#include "render_al.h"

//#define COLOR makecol(200, 200, 255)
#define COLOR makecol(color.r * 255.0f, color.g * 255.0f, color.b * 255.0f)
#define BLEND SetBlendEnabled(color.a != 1.0f, color.a * 255.0f)
#define VA_SPRINTF(text, out, last_arg) \
		va_list va; \
		char out[4096]; \
		va_start(va, last_arg); \
		vsprintf(out, text, va); \
		va_end(va);

BITMAP* gBuffer = 0;

static void SetBlendEnabled(bool enabled, int a);

void RenderAL::Init(int sx, int sy)
{
	allegro_init();
	set_color_depth(32);
	set_gfx_mode(GFX_AUTODETECT_WINDOWED, sx, sy, 0, 0);
	install_keyboard();
	install_sound(DIGI_AUTODETECT, MIDI_NONE, 0);

	gBuffer = create_bitmap(sx, sy);
}

void RenderAL::Clear()
{
	clear(gBuffer);
}

void RenderAL::Present()
{
	gKeyboard.Set(KeyCode_Left, key[KEY_LEFT]);
	gKeyboard.Set(KeyCode_Right, key[KEY_RIGHT]);
	gKeyboard.Set(KeyCode_Up, key[KEY_UP]);
	gKeyboard.Set(KeyCode_Down, key[KEY_DOWN]);
	gKeyboard.Set(KeyCode_Escape, key[KEY_ESC]);

	vsync();
	blit(gBuffer, screen, 0, 0, 0, 0, gBuffer->w, gBuffer->h);
}

void RenderAL::Fade(int amount)
{
	for (int y = 0; y < gBuffer->h; ++y)
	{
		int* line = (int*)gBuffer->line[y];

		for (int x = 0; x < gBuffer->w; ++x)
		{
			const int c = line[x];

			int r = getr32(c) - amount;
			int g = getg32(c) - amount;
			int b = getb32(c) - amount;

			if (r < 0) r = 0;
			if (g < 0) g = 0;
			if (b < 0) b = 0;

			line[x] = makecol32(r, g, b);
		}
	}
}

void RenderAL::Point(float x, float y, Color color)
{
	BLEND;

	putpixel(gBuffer, x, y, COLOR);
}

void RenderAL::Line(float x1, float y1, float x2, float y2, Color color)
{
	BLEND;

	line(gBuffer, x1, y1, x2, y2, COLOR);
}

void RenderAL::Circle(float x, float y, float r, Color color)
{
	BLEND;

	circlefill(gBuffer, x, y, r, COLOR);
}

void RenderAL::Arc(float x, float y, float angle1, float angle2, float r, Color color)
{
	BLEND;

	angle1 *= 180.0f / (float)M_PI;
	angle2 *= 180.0f / (float)M_PI;

	//arc(gBuffer, x, y, ftofix(angle1), ftofix(angle2), r, COLOR);
	circle(gBuffer, x, y, r, COLOR);
}

void RenderAL::Quad(float x1, float y1, float x2, float y2, Color color)
{
	BLEND;

	rectfill(gBuffer, x1, y1, x2, y2, COLOR);
}

void RenderAL::Text(float x, float y, Color color, const char* format, ...)
{
	BLEND;

	VA_SPRINTF(format, temp, format);

	textprintf_ex(gBuffer, font, x, y, COLOR, -1, temp);
}

static void SetBlendEnabled(bool enabled, int a)
{
	if (enabled)
	{
		drawing_mode(DRAW_MODE_TRANS, 0, 0, 0);
		set_trans_blender(255, 255, 255, a);
	}
	else
	{
		drawing_mode(DRAW_MODE_SOLID, 0, 0, 0);
	}
}
