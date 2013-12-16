#include "vecview_pch.h"
#define ALLEGRO_USE_CONSOLE
#include <allegro.h>
#include "ImageLoader_FreeImage.h"
#include "ShapeIO.h"
#include "VecRend.h"
#include "VecRend_Allegro.h"

static void Preview(BITMAP* bmp, int scale, bool invert, bool alpha, bool altBackColor)
{
	BITMAP* temp = create_bitmap_ex(32, bmp->w, bmp->h);

	blit(bmp, temp, 0, 0, 0, 0, bmp->w, bmp->h);
	
	if (altBackColor)
	{
		for (int y = 0; y < bmp->h; ++y)
		{
			int* sline = (int*)temp->line[y];
//			int* dline = (int*)temp->line[y];

			for (int x = 0; x < bmp->w; ++x)
			{
				int c = sline[x];

				int r = getr32(c);
				int g = getg32(c);
				int b = getb32(c);
				int a = geta32(c);

				r = (255 * (255 - a) + r * a) >> 8;
				g = (255 * (255 - a) + g * a) >> 8;
				b = (255 * (255 - a) + b * a) >> 8;

				putpixel(temp, x, y, makeacol32(r, g, b, a));
			}
		}
	}

	if (alpha)
	{
		for (int y = 0; y < bmp->h; ++y)
		{
			int* sline = (int*)temp->line[y];
//			int* dline = (int*)temp->line[y];

			for (int x = 0; x < bmp->w; ++x)
			{
				int c = sline[x];

				int a = geta32(c);

				putpixel(temp, x, y, makeacol32(a, a, a, a));
			}
		}
	}

	if (invert)
	{
		for (int y = 0; y < bmp->h; ++y)
		{
			int* sline = (int*)temp->line[y];
//			int* dline = (int*)temp->line[y];

			for (int x = 0; x < bmp->w; ++x)
			{
				int c = sline[x];

				int r = 255 - getr32(c);
				int g = 255 - getg32(c);
				int b = 255 - getb32(c);
				int a = 255 - geta32(c);

				putpixel(temp, x, y, makeacol32(r, g, b, a));
			}
		}
	}

	int sx = 200;
	int sy = 200;

#if 1
	if (temp->w * scale > sx)
		sx = temp->w * scale;
	if (temp->h * scale > sy)
		sy = temp->h * scale;

	int mod = 4;

	sx += mod - (sx % mod);
	sy += mod - (sy % mod);
#endif

#if 1
#ifdef WIN32
	if (set_gfx_mode(GFX_AUTODETECT_WINDOWED, sx, sy, 0, 0) < 0)
	{
		set_gfx_mode(GFX_AUTODETECT_WINDOWED, 512, 512, 0, 0);
	}
#endif
#endif

	acquire_screen();

	clear_to_color(screen, makecol(0, 255, 0));
	stretch_blit(temp, screen, 0, 0, temp->w, temp->h, 0, 0, temp->w * scale, temp->h * scale);

	release_screen();

	destroy_bitmap(temp);
}

int main(int argc, char* argv[])
{
	try
	{
		allegro_init();
		set_color_depth(32);
		//set_gfx_mode(GFX_AUTODETECT_WINDOWED, 800, 800, 0, 0);
		//set_gfx_mode(GFX_AUTODETECT_WINDOWED, 800, 600, 0, 0);
		set_gfx_mode(GFX_AUTODETECT_WINDOWED, 640, 480, 0, 0);
		install_keyboard();
		install_mouse();

		if (argc < 2)
			throw ExceptionVA("no input file given");

		ImageLoader_FreeImage imageLoader;

		Shape shape;

		if (!ShapeIO::LoadVG(argv[1], shape, &imageLoader))
			throw ExceptionVA("unable to load vector graphic");

		Buffer* buffer = VecRend_CreateBuffer(shape, 5, DrawMode_Regular);
		buffer->DemultiplyAlpha();
		BITMAP* bmp = VecRend_ToBitmap(buffer);
		delete buffer;

		bool stop = false;

		int scale = 1;
		bool invert = false;
		bool alpha = false;
		bool altBackColor = false;

		do
		{
			Preview(bmp, scale, invert, alpha, altBackColor);

			int c = readkey() & 0xFF;

			if (c >= '1' && c <= '9')
			{
				scale = c - '0';
			}
			else if (c == 'i')
			{
				invert = !invert;
			}
			else if (c == 'a')
			{
				alpha = !alpha;
			}
			else if (c == 'b')
			{
				altBackColor = !altBackColor;
			}
			else
			{
				stop = true;
			}
		} while (!stop);

		return 0;
	}
	catch (Exception e)
	{
		allegro_message(e.what());

		return -1;
	}
}
END_OF_MAIN();
