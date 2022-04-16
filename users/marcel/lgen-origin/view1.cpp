//---------------------------------------------------------------------------
// DRAWER #1
// 3d, flat, floor with textured tiles
// uses 10.6.16 fixed point math (tile_index.tex_coord.fraction)

#include "main.h"
#include "map.h"
#include "view.h"

#include "framework.h"

#include <math.h>

//---------------------------------------------------------------------------
// CUSTOM FIXED POINT MATH STUFF

#define FTOFIX(x)	int((x)*(1<<22))	// float to fix
#define FIXTOM(x)	(x>>22)&(size-1)	// fix to mapcoord
#define FIXTOT(x)	(x>>16)&63		// fix to texcoord
#define Y1		(view->h>>1)
#define Y2		view->h

//---------------------------------------------------------------------------

static BITMAP* view		= 0;
static BITMAP* img_map		= 0;
static int** tile		= 0;
static int size			= 0;

static float x, y, z;
static float ry;
static float f = M_PI*0.5;
static bool show_map = true;
static bool done = false;

//---------------------------------------------------------------------------

static void update();
static void draw();
static void init();
static void remove();
static int get_tile(int x, int y);

//---------------------------------------------------------------------------
// MAIN

void view1() {

	if (!map)
        	return;

	init();

	while (!done) {

		framework.process();
		
        	update();

			framework.beginDraw(0, 0, 0, 0);
			{
                draw();
			}
			framework.endDraw();

        }

        remove();

}

//---------------------------------------------------------------------------
// UPDATE FUNC

static void update() {

	if (keyboard.wentDown(SDLK_ESCAPE))
		done = true;
	else if (keyboard.wentDown(SDLK_F2))
		save_image(view);
	else if (keyboard.wentDown(SDLK_m))
		show_map = !show_map;

        if (keyboard.isDown(SDLK_s))
        	f -= 0.01;
        if (keyboard.isDown(SDLK_x))
        	f += 0.01;

        if (keyboard.isDown(SDLK_a))
        	y -= 0.01;
        if (keyboard.isDown(SDLK_z))
        	y += 0.01;

        int mx, my;
        mx = mouse.dx;
        my = mouse.dy;

        static float v = 0.0;
        static float vry = 0.0;

        vry -= mx*0.003;

        if (keyboard.isDown(SDLK_LEFT))
        	vry += 0.01;
        if (keyboard.isDown(SDLK_RIGHT))
        	vry -= 0.01;

	float a = 0.0;
        a += my*0.01;
        if (keyboard.isDown(SDLK_UP))
        	a -= 0.1;
        if (keyboard.isDown(SDLK_DOWN))
        	a += 0.1;

	v += a;

        vry *= 0.9;
        v *= 0.9;

        ry += vry;

        x += v*sin(ry);
        z -= v*cos(ry);;

        if (x < -map->size*2.0)
        	x = -map->size*2.0;
        if (z < -map->size*2.0)
        	z = -map->size*2.0;
        if (x > map->size*2.0)
        	x = map->size*2.0;
        if (z > map->size*2.0)
        	z = map->size*2.0;

}

//---------------------------------------------------------------------------

static inline int C(int mx, int mz, int l) {

	int tmx1 = FIXTOM(mx);
        int tmz1 = FIXTOM(mz);

	#if 1
		uint32_t c = img_tile[tile[tmx1][tmz1]]->line[FIXTOT(mz)][FIXTOT(mx)];
		uint8_t v1 = (c >> 0);
		uint8_t v2 = (c >> 8);
		uint8_t v3 = (c >> 16);
		uint8_t v4 = (c >> 24);
		v1 = (uint16_t(v1) * l) >> 8;
		v2 = (uint16_t(v2) * l) >> 8;
		v3 = (uint16_t(v3) * l) >> 8;
		v4 = (uint16_t(v4) * l) >> 8;
		return
			(uint32_t(v1) << 0) |
			(uint32_t(v2) << 8) |
			(uint32_t(v3) << 16) |
			(uint32_t(v4) << 24);
        #elif 1
	return cline[img_tile[tile[tmx1][tmz1]]->line[FIXTOT(mz)][FIXTOT(mx)]];
        #else
	return img_tile[tile[tmx1][tmz1]]->line[FIXTOT(mz)][FIXTOT(mx)];
        #endif

}

//---------------------------------------------------------------------------
// DRAWER (JAZZ JACK RABBIT STYLE) - USES 'INVERSE PROJECTIONS' OR WHATEVER

static void draw() {

        float s1 = sin(-ry-f*0.5);
        float c1 = cos(-ry-f*0.5);
        float s2 = sin(-ry+f*0.5);
        float c2 = cos(-ry+f*0.5);

        float iw = 1.0/(view->w-1.0);

        for (int sy=Y1; sy<Y2; sy+=2) {

		// unproject y coordinate

                float py = (sy-Y1)/float(view->h)*view->w/float(view->h);

                // calculate z
                // sy = y/z*dst->h*(dst->h/dst->w)+dst->h/2
                // py = (sy-dst->h/2)/(dst->h/dst->w)/dst->h
                // => py = y/z
                // z (d) = y/py

                float d = -y/py;

		float x1 = x+s1*d*20.0;
		float z1 = z+c1*d*20.0;
		float x2 = x+s2*d*20.0;
		float z2 = z+c2*d*20.0;

		int mx = FTOFIX(x1);
		int mz = FTOFIX(z1);
		int dx = FTOFIX((x2-x1)*iw);
		int dz = FTOFIX((z2-z1)*iw);

                int l = clamp<int>(int(255.0/(d+1.0)), 0, 255);

                uint32_t * addr = bmp_write_line(view, sy);
                uint32_t * addr1 = addr;
                uint32_t * addr2 = addr+view->w;

		for (uint32_t * sx=addr1; sx<addr2;) {

                        bmp_write(sx++, C(mx, mz, l));
                        mx += dx;
                        mz += dz;
                        bmp_write(sx++, C(mx, mz, l));
                        mx += dx;
                        mz += dz;
                        bmp_write(sx++, C(mx, mz, l));
                        mx += dx;
                        mz += dz;
                        bmp_write(sx++, C(mx, mz, l));
                        mx += dx;
                        mz += dz;

                }

        }

	{

	        int vx = cmap->w-128-5;
        	int vy = 5;

                if (show_map) {

	                int tx = vx+(int(z)&(map->size-1))*128/map->size;
        	        int ty = vy+(int(x)&(map->size-1))*128/map->size;

                	int l = cmap->cl;
	                int t = cmap->ct;
	                int r = cmap->cr;
        	        int b = cmap->cb;

                	set_clip(cmap, vx, vy, vx+128-1, vy+128-1);

		        blit(img_map, cmap, 0, 0, vx, vy, img_map->w, img_map->h);
		        circle(cmap, tx, ty, 3, makecol(255, 255, 255));
                	line(cmap, tx, ty, tx+int(cos(ry)*10.0), ty-int(sin(ry)*10.0), makecol(255, 255, 255));

	                set_clip(cmap, l, t, r, b);

                } else {

                	rectfill(cmap, vx, vy, vx+128-1, vy+128-1, makecol(0, 0, 0));

                }

	}

        static int frame = 0;

	#if 0 // todo : text
        text_mode(makecol(0, 0, 0));
        textprintf(cmap, font, 0, 0, makecol(255, 255, 255), "%d", frame);
	#endif

        frame++;

	swap_pages();

}

//---------------------------------------------------------------------------

static void init() {

        push_palette();

        #if !defined(DEBUG)
        fade_out(4);
        #endif

        // create palette

        set_palette_tiles();

	clear(cmap);

        PALETTE pal;
        get_palette(pal);

        #if !defined(DEBUG)
        fade_in(pal, 4);
        #endif

        #if 0
        view = create_sub_bitmap(cmap, cmap->w>>2, cmap->h>>2, cmap->w>>1, cmap->h>>1);
	#else
        view = create_sub_bitmap(cmap, 0, 0, cmap->w, cmap->h);
        #endif

	img_map = gen_radarmap(map, 128, false);

	x = z = 0.0;
        y = -0.1;
        ry = 0.0;

	done = false;

        size = map->size;

        tile = new int*[size];
        for (int i=0; i<size; i++)
        	tile[i] = new int[size];

	for (int i=0; i<size; i++)
		for (int j=0; j<size; j++)
                	tile[i][j] = get_tile(i, j);

        show_map = true;

}

//---------------------------------------------------------------------------

static void remove() {

	destroy_bitmap(img_map);
	destroy_bitmap(view);

	for (int i=0; i<size; i++)
        	delete[] tile[i];
	delete[] tile;
        tile = 0;

        #if !defined(DEBUG)
        fade_out(4);
        #endif

        pop_palette();

}

//---------------------------------------------------------------------------

static int get_tile(int x, int y) {

	return map->tile[x][y].type;

}
