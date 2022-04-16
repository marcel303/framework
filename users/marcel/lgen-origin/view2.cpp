//---------------------------------------------------------------------------
// DRAWER #3
// 2d, flat tile-based map

#include "main.h"
#include "map.h"
#include "view.h"

#include "framework.h"

//---------------------------------------------------------------------------

static void update();
static void draw();
static void init();
static void remove();

//---------------------------------------------------------------------------

static bool done = false;
static int x = 0;
static int y = 0;
static COLOR_MAP colormap;

//---------------------------------------------------------------------------
// MAIN

void view2() {

	if (!map)
        	return;

	init();

	float todo = 0.f;
	
	while (!done) {

		framework.process();
		
		todo += framework.timeStep;
		
		while (todo > 1/100.f)
		{
			todo -= 1/100.f;
			
			update();
		}

                draw();

        }

        remove();

}

//---------------------------------------------------------------------------
// UPDATE FUNC

static void update() {

	if (keyboard.wentDown(SDLK_ESCAPE))
		done = true;

        int mx, my;
        mx = mouse.dx;
        my = mouse.dy;

        x += mx;
        y += my;

#if 0 // todo : key input
        if (key[KEY_LEFT])
        	x-=10;
        if (key[KEY_UP])
        	y-=10;
        if (key[KEY_RIGHT])
        	x+=10;
        if (key[KEY_DOWN])
        	y+=10;
#endif

	if (x < 0)
        	x = 0;
	if (y < 0)
        	y = 0;
	if (x >= map->size*64-cmap->w)
        	x = map->size*64-cmap->w-1;
	if (y >= map->size*64-cmap->h)
        	y = map->size*64-cmap->h-1;

}

//---------------------------------------------------------------------------
// DRAWER

static void draw() {

	static int frame = 0;

	int x1 = x/64;
        int y1 = y/64;
        int x2 = (x+cmap->w)/64+1;
        int y2 = (y+cmap->h)/64+1;

	if (x1 < 0)
        	x1 = 0;
	if (y1 < 0)
        	y1 = 0;
	if (x2 >= map->size)
        	x2 = map->size-1;
	if (y2 >= map->size)
        	y2 = map->size-1;

	int mask = map->size-1;

        for (int i=x1; i<=x2; i++)
        	for (int j=y1; j<=y2; j++) {
//                	draw_sprite(cmap, img_tile[map->tile[j][i].type], i*64-x, j*64-y);
//                	draw_gouraud_sprite(cmap, img_tile[map->tile[j][i].type], i*64-x, j*64-y, map->tile[j][i].l, map->tile[j][(i+1)&mask].l, map->tile[(j+1)&mask][(i+1)&mask].l, map->tile[(j+1)&mask][i].l);
		 	draw_lit_sprite(cmap, img_tile[map->tile[j][i].type], i*64-x, j*64-y, map->tile[j][i].l);
                }
	mask = 0;

        int vx = cmap->w-128-5;
        int vy = 5;
        int size = map->size*64;

        rect(cmap, vx, vy, vx+128-1, vy+128-1, makecol(255, 255, 255));
        rect(cmap, vx+x*128/size, vy+y*128/size, vx+(x+cmap->w)*128/size, vy+(y+cmap->h)*128/size, makecol(255, 255, 255));

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

        // create palette

        set_palette_tiles();
        PALETTE pal;
        get_palette(pal);

        #if !defined(DEBUG)
        fade_out(4);
        #endif

	#if 0 // todo : text
	clear(screen);
	text_mode(-1);
        textprintf(screen, font, 0, 0, makecol(255, 255, 255), "please wait..");
	#endif

        #if !defined(DEBUG)
        fade_in(pal, 4);
        #endif

	done = false;
        x = y = 0;

        for (int i=0; i<256; i++) {
		for (int j=0; j<256; j++) {
                	colormap.data[j][i] = makecol((pal[i].r*j)>>6, (pal[i].g*j)>>6, (pal[i].b*j)>>6);
                }
        }

        color_map = &colormap;

#if 0 // todo : screen
	clear(screen);
#endif

}

//---------------------------------------------------------------------------

static void remove() {

        #if !defined(DEBUG)
        fade_out(4);
        #endif

        pop_palette();

}
