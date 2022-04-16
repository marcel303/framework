#pragma once

#include <stdint.h>

struct BITMAP
{
	// storage
	
	uint32_t * data = nullptr;
	
	// storage size
	
	int w = 0;
	int h = 0;
	
	// access
	
	uint32_t * line[1024] = { };
	
	// clip rect
	
	int cl = 0;
	int cr = 0;
	int ct = 0;
	int cb = 0;
};

struct PALETTE_ENTRY
{
	int r;
	int g;
	int b;
	int a;
};

typedef PALETTE_ENTRY PALETTE[256];

struct COLOR_MAP
{
	uint8_t data[256][256];
};

extern COLOR_MAP * color_map;

BITMAP * create_bitmap(int sx, int sy);
BITMAP * create_sub_bitmap(BITMAP * src, int x, int y, int sx, int sy);
void destroy_bitmap(BITMAP * bmp);
BITMAP * load_bitmap(const char * path, PALETTE pal);
int save_bitmap(const char * path, BITMAP * bmp, PALETTE pal);
void set_clip(BITMAP * bmp, int cl, int ct, int cr, int cb);

int makecol(int r, int g, int b);
uint32_t makeacol(int r, int g, int b, int a);

int getpixel(BITMAP * bmp, int x, int y);

void clear(BITMAP * bmp);
void blit(BITMAP * src, BITMAP * dst, int src_x, int src_y, int dst_x, int dst_y, int sx, int sy);
void draw_sprite(BITMAP * dst, BITMAP * src, int x, int y);
void draw_lit_sprite(BITMAP * dst, BITMAP * src, int x, int y, int c);
void draw_gouraud_sprite(BITMAP * dst, BITMAP * src, int x, int y, int l1, int l2, int l3, int l4);

void circle(BITMAP * bmp, int x, int y, int r, int c);
void rect(BITMAP * bmp, int x1, int y1, int x2, int y2, int c);
void rectfill(BITMAP * bmp, int x1, int y1, int x2, int y2, int c);
void line(BITMAP * bmp, int x1, int y1, int x2, int y2, int c);

uint32_t * bmp_write_line(BITMAP * bmp, int y);
void bmp_write(uint32_t * addr, int c);

void rgb_to_hsv(int r, int g, int b, float * h, float * s, float * v);
void hsv_to_rgb(float h, float s, float v, int * r, int * g, int * b);

void get_palette(PALETTE pal);
void set_palette(PALETTE pal);
void set_palette_range(PALETTE pal, int begin, int end, int vsync);
