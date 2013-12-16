#define ALLEGRO_NO_MAGIC_MAIN
#include <allegro.h>
#include <cmath>
#include <stdio.h>

// tweakables

#define HQ 0

static const int W = 256 * 2;
static const int H = 256 * 2;

typedef unsigned char COLOR_VALUE;
typedef short         WATER_VALUE;

#define FSIN(_x) fixsin((_x) << 13)
#define FCOS(_x) fixcos((_x) << 13)

// options

static struct
{
	bool surfer;
	bool raindrops;
	bool blobs;
	bool swirl;
	bool movement;
	int density;
}
options =
	{
		false,
		false,
		true,
		true,
		true,
		6
	};

static WATER_VALUE values[2][W * H];  // height values
static COLOR_VALUE background[W * H]; // background image data
static PALETTE palette;               // palette
static BITMAP * composite = 0;        // composite background + water image

static bool load_background_image(); // load background image and make it fit WxH
static void loop();                  // main loop
static void display();               // blit composite image to screen

static void render_water(int index, int light);   // create composite image and display
static void update_water(int index, int density); // update the height values
static void smooth_water(int index);              // smooth out the water

static void render_distblob(int x, int y, int radius, int height, int index); // blob that uses a distance function
static void render_sineblob(int x, int y, int radius, int height, int index); // blob that uses a sine function
static void render_box(int x, int y, int radius, int height, int indx);       // box around edges

static void error(const char * msg, ...)
{
	va_list va;
	va_start(va, msg);
	char temp[1024];
	vsprintf_s(temp, sizeof(temp), msg, va);
	va_end(va);
	allegro_message(temp);
	exit(-1);
}

int main(int argc, char * argv[])
{
	allegro_init();
	
	if (install_mouse() < 0)
	{
		error("unable to install mouse driver");
	}

	if (install_keyboard() < 0)
	{
		error("unable to install keyboard driver");
	}

	set_color_depth(8);

	if (set_gfx_mode(GFX_AUTODETECT_WINDOWED, W, H, 0, 0) < 0)
	{
		error("unable to set graphics mode: %d x %d @ %d BPP", W, H, 8);
	}

	if (!load_background_image())
	{
		error("unable to load background image");
	}

	set_palette(palette);

	loop();

	allegro_exit();

	return 0;
}
END_OF_MAIN();

typedef struct
{
	int index;
	float lightness;
} lightness_t;

static int lightness_callback(const void * __restrict e1, const void * __restrict e2)
{
	const lightness_t * __restrict l1 = (const lightness_t *)e1;
	const lightness_t * __restrict l2 = (const lightness_t *)e2;
	
	return l1->lightness - l2->lightness < 0.0 ? -1 : l1->lightness == l2->lightness ? 0 : 1;
}

static bool load_background_image()
{
	BITMAP * bmp = load_bitmap("texture.bmp", palette);

	if (bmp == 0)
		return false;

	int conversion[256];

	lightness_t lightness[256];

	for (int i = 0; i < 256; ++i)
	{
		const int r = palette[i].r * 255 / 63;
		const int g = palette[i].g * 255 / 63;
		const int b = palette[i].b * 255 / 63;

		float hue, saturation;
		rgb_to_hsv(r, g, b, &hue, &saturation, &lightness[i].lightness);
		lightness[i].index = i;
	}

	qsort(lightness, 256, sizeof(lightness_t), lightness_callback);

	for (int i = 0; i < 256; ++i)
	{
		conversion[lightness[i].index] = i;
	}

	// update palette & texture

	PALETTE temp;

	for (int i = 0; i < 256; ++i)
	{
		temp[conversion[i]] = palette[i];
	}

	for (int i = 0; i < 256; ++i)
	{
		palette[i] = temp[i];
	}

	for (int i = 0; i < bmp->w; ++i)
	{
		for (int j = 0; j < bmp->h; ++j)
		{
			const int c = getpixel(bmp, i, j);

			putpixel(bmp, i, j, conversion[c]);
		}
	}

	// copy tiled / cropped

	for (int i = 0; i < W; ++i)
		for (int j = 0; j < H; ++j)
			background[i + j * W] = getpixel(bmp, i % bmp->w, j % bmp->h);

	destroy_bitmap(bmp);
	bmp = 0;

	return true;
}

void loop()
{
	int mapIndex   = 0;
	int swirlangle = rand() & 2047;
	int force      = 400;
	int radius     = 30;
	int light      = 0;

	memset(values[0], 0, sizeof(WATER_VALUE)* W * H);
	memset(values[1], 0, sizeof(WATER_VALUE)* W * H);

	composite = create_bitmap_ex(8, W, H);
	clear(composite);

	set_mouse_sprite(0);
	show_mouse(screen);

	static int frame = 0;
	
	while (key[KEY_ESC] == 0)
	{
		if (mouse_b & 1)
			render_sineblob(mouse_x, mouse_y, radius, -force, mapIndex);

		if (mouse_b & 2)
			render_sineblob(mouse_x, mouse_y, radius, +force, mapIndex);

		while (keypressed())
		{
			const int key = readkey() & 0xFF;

			switch (key)
			{
				// adjust density
				case 'a': options.density--; break;
				case 'z': options.density++; break;

				// clear buffers
				case 'c':
					memset(values[0], 0, sizeof(WATER_VALUE) * W * H);
					memset(values[1], 0, sizeof(WATER_VALUE) * W * H);
					break;

				// MODE: water
				case 'q':
					options.density = 4;
					light           = 1;
					force           = 600;
					break;

				// MODE: jelly
				case 'w':
					options.density = 3;
					force           = 400;
					light           = 0;
					break;

				// MODE: extra jelly
				case 'e':
					options.density = 6;
					force           = 400;
					light           = 0;
					break;

				// MODE: extreme jelly
				case 'r':
					options.density = 8;
					force           = 400;
					light           = 0;
					break;

				// MODE: very blobby
				case 't':
					options.density = 4;
					force           = 1400;
					radius          = 80; 
					light           = 1;
					break;

				// toggle movement
				case 'p':
					options.movement = !options.movement;
					if (options.movement)
						force = 400;
					else
						force = 256;
					break;

				case '1':
					options.surfer = !options.surfer;
					break;
				case '2':
					options.raindrops = !options.raindrops;
					break;
				case '3':
					options.blobs = !options.blobs;
					break;
				case '4':
					options.swirl = !options.swirl;
					break;
			}
		}

		// EFFECT: surfer
		if (options.surfer)
		{
			static int r1 = 0;
			static int r2 = 0;

			for (int i = 0; i < 2; i++)
			{
				const int x = (W / 2) +
					(
						(
							(
								(FSIN((r1 * 65 ) >> 8) >> 8) *
								(FSIN((r1 * 349) >> 8) >> 8)
							) * W / 3
						) >> 16
					);
				const int y = (H / 2) +
					(
						(
							(
								(FSIN((r2 * 377) >> 8) >> 8) *
								(FSIN((r2 * 84 ) >> 8) >> 8)
							) * H / 3
						) >> 16
					);

				r1 += 7;
				r2 += 9;

				render_sineblob(x, y, 4, -2000, mapIndex);
			}
		}

		// EFFECT: raindrops
		if (options.raindrops)
		{
			const int x = rand() % (W - 10) + 5;
			const int y = rand() % (H - 10) + 5;
			values[mapIndex][y * W + x] = rand() % (force << 2);
		}

		// EFFECT: blobs
		if (options.blobs)
		{
			if(rand() % 40 == 0)
				render_sineblob(-1, -1, radius, -force * 6, mapIndex);
		}

		// EFFECT: swirl
		if (options.swirl && (frame/256) & 1)
		{
			const int x = (W >> 1) +
				(
					(
						(FCOS(swirlangle)) * 25
					) >> 16
				);
			const int y = (H >> 1) +
				(
					(
						(FSIN(swirlangle)) * 25
					) >> 16
				);

			swirlangle += 20;
			render_distblob(x, y, radius, force, mapIndex);
		}

		render_water(mapIndex, light);

		const int newIndex = (mapIndex + 1) % 2;

		if (options.movement)
		{
			update_water(newIndex, options.density);
		}
		else
		{
			WATER_VALUE * __restrict newPtr = values[newIndex];
			WATER_VALUE * __restrict oldPtr = values[mapIndex];
			for (int i = W * H; i != 0; --i)
				*newPtr++ = *oldPtr++;
		}

		mapIndex = newIndex;

		frame++;
	}

	destroy_bitmap(composite);
	composite = 0;
}

void render_water(int index, int light)
{
	int offset = W + 1;

	COLOR_VALUE * __restrict bptr = background;

	for (int y = 1; y < H - 1; ++y)
	{
		COLOR_VALUE * __restrict cptr = composite->line[y];
		WATER_VALUE * __restrict sptr = values[index] + y * W;

		for (int x = 0; x < W; ++x)
		{
			const int dx = sptr[0] - sptr[1];
			const int dy = sptr[0] - sptr[W];

			++sptr;

			const int tx = (x + (dx >> 3)) & (W - 1);
			const int ty = (y + (dy >> 3)) & (H - 1);

			const int intensity = dx >> light;

			int c = bptr[W * ty + tx] + intensity;

			if (c < 0)   c = 0;
			if (c > 255) c = 255;

			*cptr++ = c;
		}
	}

	display();
}

void update_water(int index, int density)
{
	const int p1 = (W + 1);
	const int p2 = (H - 1) * W;

	const int newIndex = (index + 0) % 2;
	const int oldIndex = (index + 1) % 2;

	for (int p = p1; p < p2; p += 2)
	{
		WATER_VALUE * __restrict newPtr = values[newIndex] + p;
		WATER_VALUE * __restrict oldPtr = values[oldIndex] + p;

		for (int p3 = p + W - 2; p < p3; ++p)
		{
		#if HQ
			const int newValue =
				(
					(
						oldPtr[+ W - 1] +
						oldPtr[+ W + 0] +
						oldPtr[+ W + 1] +
						oldPtr[- W - 1] +
						oldPtr[- W + 0] +
						oldPtr[- W + 1] +
						oldPtr[+ 0 + 1] +
						oldPtr[+ 0 - 1]
					) >> 2
				) - (*newPtr);
		#else
			const int newValue =
				(
					(
						oldPtr[+ W + 0] +
						oldPtr[- W + 0] +
						oldPtr[+ 0 + 1] +
						oldPtr[+ 0 - 1]
					) >> 1
				) - (*newPtr);
		#endif

			*newPtr++ = newValue - (newValue >> density);

			oldPtr++;
		}
	}
}

void smooth_water(int index)
{
	const int newIndex = (index + 0) % 2;
	const int oldIndex = (index + 1) % 2;

	for (int y = 1; y < H - 1; y++)
	{
		WATER_VALUE * __restrict newPtr = values[newIndex] + y * W + 1;
		WATER_VALUE * __restrict oldPtr = values[oldIndex] + y * W + 1;

		for (int x = 1; x < W - 1; x++)
		{
			const int newh =
				(
					(
						oldPtr[+ W + 0] +
						oldPtr[- W + 0] +
						oldPtr[+ 1 + 0] +
						oldPtr[- 1 + 0] +
						oldPtr[- W - 1] +
						oldPtr[- W + 1] +
						oldPtr[+ W - 1] +
						oldPtr[+ W + 1]
					) >> 3
				) + newPtr[0];

			newPtr[0] = newh >> 1;

			newPtr++;
			oldPtr++;
		}
	}
}

void render_distblob(int x, int y, int radius, int height, int index)
{
	int radiusSq = radius * radius;

	int x1 = x - radius;
	int y1 = y - radius;
	int x2 = x + radius;
	int y2 = y + radius;

	if (x1 < 0)
		x1 = 0;
	if (y1 < 0)
		y1 = 0;

	if (x2 >= W)
		x2 = W - 1;
	if (y2 >= H)
		y2 = H - 1;

	for (int py = y1; py <= y2; py++)
	{
		for (int px = x1; px <= x2; px++)
		{
			const int dx = px - x;
			const int dy = py - y;
			const int dSq = dy * dy + dx * dx;

			if (dSq < radiusSq)
			{
				values[index][py * W + px] += int((radius - std::sqrtf(dSq)) * height) >> 6;
			}
		}
	}
}

void render_sineblob(int x, int y, int radius, int height, int index)
{
	float length = (1024.f / radius) * (1024.f / radius);

	if (x < 0)
		x = 1 + radius + rand() % (W - 2 * radius - 1);
	if (y < 0)
		y = 1 + radius + rand() % (H - 2 * radius - 1);

	int radiusSq = radius * radius;

	int x1 = -radius;
	int y1 = -radius;
	int x2 = +radius;
	int y2 = +radius;

	int px1 = x + x1;
	int py1 = y + y1;
	int px2 = x + x2;
	int py2 = y + y2;

	if (px1 < 1    ) x1 -= px1 - (1    );
	if (py1 < 1    ) y1 -= py1 - (1    );
	if (px2 > W - 1) x2 -= px2 - (W - 1);
	if (py2 > H - 1) y2 -= py2 - (H - 1);

	for (int dy = y1; dy < y2; ++dy)
	{
		for (int dx = x1; dx < x2; ++dx)
		{
			const int dSq = dx * dx + dy * dy;

			if (dSq < radiusSq)
			{
				const int dist = (int)std::sqrtf(dSq * length);

				values[index][W * (dy + y) + dx + x] += ((FCOS(dist)+0xFFFF) * (height)) >> 19;
			}
		}
	}
}

void render_box(int x, int y, int radius, int height, int index)
{
	if (x < 0)
		x = 1 + radius + rand() % (W - 2 * radius - 1);
	if (y < 0)
		y = 1 + radius + rand() % (H - 2 * radius - 1);

	int x1 = -radius;
	int y1 = -radius;
	int x2 = +radius;
	int y2 = +radius;

	int px1 = x + x1;
	int py1 = y + y1;
	int px2 = x + x2;
	int py2 = y + y2;

	if (px1 < 1    ) x1 -= px1 - (1    );
	if (py1 < 1    ) y1 -= py1 - (1    );
	if (px2 > W - 1) x2 -= px2 - (W - 1);
	if (py2 > H - 1) y2 -= py2 - (H - 1);

	for (int dy = y1; dy < y2; ++dy)
	{
		for (int dx = x1; dx < x2; ++dx)
		{
			values[index][(y + dy) * W + (x + dx)] = height;
		}
	}
}

static void display()
{
	//vsync();

	blit(composite, screen, 0, 0, 0, 0, W, H);

	{
		static int frame = 0;
		textprintf_ex(screen, font, 0, 0, 0, 255, "%d", frame);
		frame++;
	}
}
