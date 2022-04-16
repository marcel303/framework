//---------------------------------------------------------------------------
// LANDHACK COMPETITION ENTRY - MARCEL SMIT - 2001
//---------------------------------------------------------------------------

#include "dialog.h"
#include "main.h"
#include "map.h"
#include "view.h"

#include "framework.h"

#include "allegro-compat.h"

#include <math.h>
#include <stdio.h>
#include <time.h>

//---------------------------------------------------------------------------

#if !defined(ALLEGRO_DJGPP) && !defined(ALLEGRO_MINGW32)
#warning compiling for an untested platform! it is possible you will run into unexpected trouble..
#endif

//---------------------------------------------------------------------------

//#define USE_VRAM	// draw directly to screen?

#define PAL_COL(_p, _i, _r, _g, _b)	_i = makecol(_r, _g, _b)

//---------------------------------------------------------------------------

#define MAP_SIZE	256
#define MAX_TILES	8

//---------------------------------------------------------------------------
// PROGRAM FLOW FLAGS

#define F_EXIT		0x1
#define F_DRAW_BG	0x2
#define F_DRAW_MAP	0x4
#define F_DRAW_MENU	0x8
#define F_DRAW		(F_DRAW_BG|F_DRAW_MAP|F_DRAW_MENU)

//---------------------------------------------------------------------------
// MENU DEFINES

#define M_ITEM_H	32
#define M_ITEMS		7
#define M_GENERATE	0
#define M_SAVE		1
#define M_VIEW_FLAT	2
#define M_VIEW_3D	3
#define M_REDRAW	4
#define M_OPTIONS	5
#define M_EXIT		6

//---------------------------------------------------------------------------
// PALETTE STARTING POINTS (FOR COLOUR INDEX TRANSLATIONS ETC)

#define P_STD		0
#define P_MAIN		32
#define P_BACK          96
#define P_LAND          128
#define P_MYNAME	160

//---------------------------------------------------------------------------
// STANDARD COLOURS

int C_BLACK = 0;
int C_WHITE = 0;
int C_RED = 0;
int C_GREEN = 0;
int C_BLUE = 0;
int C_DK_GRAY = 0;
int C_GRAY = 0;
int C_LT_GRAY = 0;
int C_BLUE_F = 0;
int C_BLUE_B = 0;

//---------------------------------------------------------------------------
// LANDSCAPE COLOURS

int C_WATER = 0;
int C_LAND = 0;
int C_ROCK = 0;
int C_PLAYER = 0;

//---------------------------------------------------------------------------

BITMAP* cmap			= 0;
static BITMAP* lmap		= 0;
static BITMAP* img_main		= 0;
static BITMAP* img_back		= 0;
static BITMAP* img_myname	= 0;
static PALETTE palette_main;
static PALETTE palette_tiles;
BITMAP** img_tile		= 0;
int flags			= 0;
mapgen_config_t config;

//---------------------------------------------------------------------------

static void update();
static void draw();

static void draw_map();

static bool init();
static bool shutdown();
void swap_pages();

static void generate();
static int update_menu();
static void draw_menu();
static void translate_image(BITMAP* bmp, int base, bool masked);

//---------------------------------------------------------------------------

int main(int argc, char* argv[]) {

        if (!init())
        	exit(-2);

	while (!(flags & F_EXIT)) {

			framework.process();
			
        	update();
        	
        	framework.beginDraw(0, 0, 0, 0);
        	{
                draw();
			}
			framework.endDraw();

        }

	if (!shutdown())
        	exit(-2);

	return 0;

}

//---------------------------------------------------------------------------
// UPDATE/LOGIC FUNCTION

static void update() {

	// handle input

#if 0 // todo : key input
	while (keypressed()) {

		int c = readkey()>>8;

                switch(c) {
                        case KEY_F1:
				generate();
                        break;
                        case KEY_F2:
                        	save_image(cmap);
                        break;
                        case KEY_F5:
                        	flags |= F_DRAW;
                        break;
                        case KEY_F8:
                        	dlg_config();
                        break;
                        case KEY_1:
                        	view1();
                                flags |= F_DRAW;
                        break;
                        case KEY_3:
                        	view3();
                                flags |= F_DRAW;
                        break;
                	case KEY_ESC:
                        	#if !defined(DEBUG)
                        	if (dlg_message("are you sure you want to quit?", DLG_YESNO))
				#endif
	                        	flags |= F_EXIT;
			break;
                        #if !defined(DEBUG)
                        default:
                        	dlg_message("unassigned key", DLG_OK);
                        break;
                        #endif
                }

                clear_keybuf();

	}
#endif

        int b = update_menu();

        if (b >= 0) {

        	switch(b) {
                	case M_GENERATE:
                        	generate();
                        break;
                        case M_SAVE:
                        	save_image(cmap);
                        break;
                        case M_VIEW_FLAT:
                        	view3();
                                flags |= F_DRAW;
                        break;
                        case M_VIEW_3D:
                        	view1();
                                flags |= F_DRAW;
                        break;
                        case M_REDRAW:
                        	flags |= F_DRAW;
                        break;
                        case M_OPTIONS:
                        #if 0 // todo : dialog
                        	dlg_config();
						#endif
                        break;
                	case M_EXIT:
                        	#if !defined(DEBUG)
                        	if (dlg_message("are you sure you want to quit?", DLG_YESNO))
				#endif
	                        	flags |= F_EXIT;
			break;
                }

        }

        static int t = 0;
        t++;
        PALETTE tmp;
        for (int i=P_BACK; i<P_BACK+32; i++) {
        	float h, s, v;
        	rgb_to_hsv(palette_main[i].r<<2, palette_main[i].g<<2, palette_main[i].b<<2, &h, &s, &v);
                h = t*0.5;
                s *= (sin(t*0.00100)+1.0)*0.5;
                v *= (sin(t*0.00333)+1.0)*0.5;
                int r, g, b;
                hsv_to_rgb(h, s, v, &r, &g, &b);
                tmp[i].r = r>>2;
                tmp[i].g = g>>2;
                tmp[i].b = b>>2;
        }
        set_palette_range(tmp, P_BACK, P_BACK+32-1, 1);

}

//---------------------------------------------------------------------------
// DRAWING FUNCTION

static void draw() {

	flags |= F_DRAW;
	
	if (!(flags & F_DRAW))
        	return;

	if (flags & F_DRAW_BG) {

        	flags -= F_DRAW_BG;

                int repx = cmap->w/img_back->w+1;
                int repy = cmap->h/img_back->h+1;

                for (int i=0; i<repx; i++)
                	for (int j=0; j<repy; j++)
                        	blit(img_back, cmap, 0, 0, i*img_back->w, j*img_back->h, img_back->w, img_back->h);

                draw_sprite(cmap, img_myname, cmap->w-img_myname->w-5, cmap->h-img_myname->h-5);

	}

	if (flags & F_DRAW_MENU) {

        	flags -= F_DRAW_MENU;

		draw_menu();

	}

        // map

        if (flags & F_DRAW_MAP) {

                flags -= F_DRAW_MAP;

	        draw_map();

	}

	swap_pages();

}

//---------------------------------------------------------------------------
// DRAWS A SIMPLE MAP VIEW

static void draw_map() {

	int y = (cmap->h-512)>>1;
	int x = 200+(cmap->w-512-200)/2;
        int c[3] = { C_WATER, C_LAND, C_ROCK };

        for (int i=0; i<(map?map->size:lmap->w); i++) {

        	uint32_t * lline = bmp_write_line(lmap, i);

                if (!map) {

	        	for (int j=0; j<lmap->w;) {

                        	// unrolled

        	        	bmp_write(lline, (i+j)&15);
                	        lline++;
                                j++;
        	        	bmp_write(lline, (i+j)&15);
                	        lline++;
                                j++;
        	        	bmp_write(lline, (i+j)&15);
                	        lline++;
                                j++;
        	        	bmp_write(lline, (i+j)&15);
                	        lline++;
                                j++;
			}

		} else {

                	// convert height values to image
                        // NOTE: this will swap (x, y)!

	                tile_t* tline = map->tile[i];

	        	for (int j=0; j<map->size;) {

        	        	bmp_write(lline, c[tline->type]);
                	        lline++;
                                tline++;
                                j++;
        	        	bmp_write(lline, c[tline->type]);
                	        lline++;
                                tline++;
                                j++;
        	        	bmp_write(lline, c[tline->type]);
                	        lline++;
                                tline++;
                                j++;
        	        	bmp_write(lline, c[tline->type]);
                	        lline++;
                                tline++;
                                j++;

			}

                }
	}

        // draw players

        if (map)
	        for (int i=0; i<map->players; i++)
        		circle(lmap, map->player[i].y, map->player[i].x, 5, C_PLAYER);

        // fill 512x512 area

        int rep;
        if (map)
	        rep = 512/map->size;
	else
        	rep = 2;

	int size = map?map->size:256;

        for (int i=0; i<rep; i++)
        	for (int j=0; j<rep; j++) {
		        blit(lmap, cmap, 0, 0, x+i*size, y+j*size, size, size);
                        rect(cmap, x+i*size, y+j*size, x+(i+1)*size-1, y+(j+1)*size-1, C_BLACK);
		}

	rect(cmap, x-1, y-1, x+512, y+512, C_WHITE);

}

//---------------------------------------------------------------------------

bool save_image(BITMAP* bmp) {

	int index = 0;
        bool done = false;
       	char filename[32];

        // find a free filename

	while (index <= 999 && !done) {

                sprintf(filename, "save%03d.bmp", index);
	#if 0 // todo : filesystem
		if (!exists(filename)) {
                	done = true;
                } else
	        	index++;
	#endif

        }

        // no filename found

        if (!done) {
        #if 0 // todo : dialog
        	dlg_message("error: entire 000-999 range is full!!", DLG_OK);
		#endif
        	return false;
	}

        // save

        PALETTE pal;
        get_palette(pal);
	if (save_bitmap(filename, bmp, pal)) {
		#if 0 // todo : dialog
        	dlg_message("error: unable to save image!", DLG_OK);
		#endif
                return false;
        } else {
	        #if !defined(DEBUG)
		char tmp[64];
		sprintf(tmp, "info: image saved to %s", filename);
       		dlg_message(tmp, DLG_OK);
	        #endif
        }

	return true;

}

//---------------------------------------------------------------------------

static bool init() {

	setupPaths(CHIBI_RESOURCE_PATHS);
	
	// uclock is available under DJGPP, don't klnow about the rest..

	#if defined(ALLEGRO_DJGPP)
	srand(uclock());
	#endif

        // initialize framework

	framework.init(800, 600);

	// setup palettes

	// standard colors (0..31)

	PAL_COL(main, C_BLACK,		0,	0,	0	);
        PAL_COL(main, C_WHITE,		255,	255,	255	);
        PAL_COL(main, C_RED,		255,	0,	0	);
        PAL_COL(main, C_GREEN,		0,	255,	0	);
        PAL_COL(main, C_BLUE,		0,	0,	255	);
        PAL_COL(main, C_DK_GRAY,	63,	63,	63	);
        PAL_COL(main, C_GRAY,		127,	127,	127	);
        PAL_COL(main, C_LT_GRAY,	191,	191,	191	);
        PAL_COL(main, C_BLUE_F,		63,	127,	255	);
        PAL_COL(main, C_BLUE_B,		31,	63,	127	);

        // map colors (96..255)

	PAL_COL(main, C_WATER,	63,	127,	255	);
	PAL_COL(main, C_LAND,	127,	95,	63	);
	PAL_COL(main, C_ROCK,	127,	127,	149	);
        PAL_COL(main, C_PLAYER,	255,	255, 	0	);

        // create color map (draw direct to screen or back buffer..)

        #ifdef USE_VRAM
        cmap = create_sub_bitmap(screen, 0, 0, 800, 600);
        #else
        cmap = create_bitmap(800, 600);
        #endif

        if (!cmap) {
        #if 0 // todo : dialog
        	dlg_message("error: unable to create buffer", DLG_OK);
		#endif
                return false;
	}

        // map image

        lmap = create_bitmap(512, 512);
        if (!lmap) {
        #if 0 // todo : dialog
        	dlg_message("error: unable to create buffer for map", DLG_OK);
		#endif
                return false;
        }

        // main menu image

        PALETTE tmp;
        img_main = load_bitmap("./main.bmp", tmp);
        if (!img_main) {
        #if 0 // todo : dialog
        	dlg_message("error: unable to load main graphic", DLG_OK);
		#endif
                return false;
        }
        translate_image(img_main, P_MAIN, false);

        // background image

        img_back = load_bitmap("./back.bmp", tmp);
        if (!img_back) {
        #if 0 // todo : dialog
        	dlg_message("error: unable to load background graphic", DLG_OK);
		#endif
                return false;
        }
        translate_image(img_back, P_BACK, false);

        // my name image

        img_myname = load_bitmap("./myname.bmp", tmp);
        if (!img_myname) {
        #if 0 // todo : dialog
        	dlg_message("error: unable to load myname graphic", DLG_OK);
		#endif
                return false;
        }
        translate_image(img_myname, P_MYNAME, true);

	// tiles

	{

        	BITMAP* tmp = load_bitmap("./tiles.bmp", palette_tiles);
                if (!tmp) {
			#if 0 // todo : dialog
	        	dlg_message("error: unable to load tiles graphic", DLG_OK);
			#endif
                	return false;
		}

                translate_image(tmp, 32, false);

                for (int i=255; i>=32; i--) {
                	palette_tiles[i].r = palette_tiles[i-32].r;
                	palette_tiles[i].g = palette_tiles[i-32].g;
                	palette_tiles[i].b = palette_tiles[i-32].b;
                }
                for (int i=0; i<32; i++)
                	palette_tiles[i] = palette_main[i];

                img_tile = new BITMAP*[MAX_TILES];
                for (int i=0; i<MAX_TILES; i++) {
                	img_tile[i] = create_bitmap(64, 64);
                        blit(tmp, img_tile[i], i*64, 0, 0, 0, 64, 64);
                }

        }

        config.algo = MAP_OFFSET_SQUARE;
        config.filt_median = true;
        config.filt_min = false;
        config.filt_max = false;
        config.filt_diff = false;
        config.filt_mean = true;
        config.filt_size = 5;
        config.w = 0.0;
        config.l = 0.45;
        config.r = 0.65;
        config.players = 3;

        set_palette_main();

	// todo : dialog : dlg_init();

        flags = F_DRAW;

	return true;

}

//---------------------------------------------------------------------------

static bool shutdown() {

	for (int i=0; i<MAX_TILES; i++)
        	destroy_bitmap(img_tile[i]);
	delete[] img_tile;		img_tile	= 0;
        destroy_bitmap(img_myname);	img_myname	= 0;
	destroy_bitmap(img_back);	img_back	= 0;
	destroy_bitmap(img_main);	img_main	= 0;
        destroy_bitmap(lmap);		lmap		= 0;
        destroy_bitmap(cmap);		cmap		= 0;

	framework.shutdown();

        return true;

}

//---------------------------------------------------------------------------

void swap_pages() {

	// copy color map

	pushBlend(BLEND_OPAQUE);
	{
		auto texture = createTextureFromRGBA8(cmap->data, cmap->w, cmap->h, false, true);
		gxSetTexture(texture, GX_SAMPLE_NEAREST, true);
		setColor(colorWhite);
		drawRect(0, 0, cmap->w, cmap->h);
		gxClearTexture();
		freeTexture(texture);
	}
	popBlend();

}

//---------------------------------------------------------------------------

static void generate() {

	PALETTE pal;
	get_palette(pal);

        // fade palette

	#if !defined(DEBUG)
        bool fade = map?true:false;
        if (fade)
        	fade_out_range(4, P_LAND, P_LAND+31);
	#endif

	map_gen(config.algo, MAP_SIZE);

	flags |= F_DRAW_MAP;

        // restore palette

	#if !defined(DEBUG)
	draw();
	if (fade)
		fade_in_range(pal, 4, P_LAND, P_LAND+31);
	#endif

}

//---------------------------------------------------------------------------
// MENU

typedef struct {
        bool inside;
	bool down;
} menu_item_t;

static int menu_item_y[M_ITEMS] = { 28, 60, 108, 140, 188, 220, 268 };
static menu_item_t menu_item[M_ITEMS];

static int update_menu() {

	int x = mouse.x;
        int y = mouse.y;

        static bool down = false;
        bool click;

        if (mouse.isDown(BUTTON_LEFT) && !down) {

        	click = true;
                down = true;

        } else {

        	click = false;
                if (!mouse.isDown(BUTTON_LEFT))
	        	down = false;

        }

        int ret = -1;

	for (int i=0; i<M_ITEMS; i++) {

		if (x >= 0 && x <= 200-1 && y >= menu_item_y[i] && y <= menu_item_y[i]+M_ITEM_H-1) {

                	if (!menu_item[i].inside)
	                	flags |= F_DRAW_MENU;
                	menu_item[i].inside = true;

		} else {

                	if (menu_item[i].inside)
	                	flags |= F_DRAW_MENU;
                	menu_item[i].inside = false;

		}

        	if (click) {

	        	if (menu_item[i].inside)
                        	menu_item[i].down = true;
			else
				menu_item[i].down = false;

		} else if (!down) {

                	if (menu_item[i].down && menu_item[i].inside)
	                        ret = i;
			menu_item[i].down = false;

                }

        }

        return ret;

}

static void draw_menu() {

	blit(img_main, cmap, 0, 0, 0, 0, img_main->w, img_main->h);

        // draw item under cursor

        for (int i=0; i<M_ITEMS; i++)
        	if (menu_item[i].inside)
                       	rect(cmap, 0, menu_item_y[i], 200-1, menu_item_y[i]+M_ITEM_H-1, makecol(255, 255, 255));

}

//---------------------------------------------------------------------------
// PALETTES

static PALETTE palette_stack[10];
static int palette_depth = 0;

void push_palette() {

	get_palette(palette_stack[palette_depth]);
        palette_depth++;

}

void pop_palette() {

	palette_depth--;
        set_palette(palette_stack[palette_depth]);

}

void set_palette_main() {

	set_palette(palette_main);

}

void set_palette_tiles() {

	set_palette(palette_tiles);

}

//---------------------------------------------------------------------------

static void translate_image(BITMAP* bmp, int base, bool masked) {

	uint32_t* ptr = bmp->line[0];

        for (uint32_t* ptr2=ptr+bmp->w*bmp->h; ptr<ptr2;) {

        	if (!masked || *ptr)
			*ptr += base;
		ptr++;

        	if (!masked || *ptr)
			*ptr += base;
		ptr++;

        }

}
