#ifndef __main_h__
#define __main_h__

//---------------------------------------------------------------------------

#include "allegro-compat.h"

//---------------------------------------------------------------------------

#define TILE_WATER_START	0
#define TILE_LAND_START		1
#define TILE_ROCK_START		2

typedef struct {
	int algo;
        bool filt_median;
        bool filt_min;
        bool filt_max;
        bool filt_diff;
        bool filt_mean;
        int filt_size;
        float w;
        float l;
        float r;
        int players;
} mapgen_config_t;

//---------------------------------------------------------------------------

extern BITMAP* cmap;
extern BITMAP** img_tile;
extern mapgen_config_t config;

//---------------------------------------------------------------------------

extern bool save_image(BITMAP* bmp);
extern void swap_pages();
extern void push_palette();
extern void pop_palette();
extern void set_palette_main();
extern void set_palette_tiles();

//---------------------------------------------------------------------------

#endif
