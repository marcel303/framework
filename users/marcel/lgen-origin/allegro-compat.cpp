#include "allegro-compat.h"

#include "framework.h"
#include "image.h"

COLOR_MAP * color_map = nullptr;

BITMAP * create_bitmap(int sx, int sy)
{
	BITMAP * dst = new BITMAP();
	
	dst->data = new uint32_t[sx * sy];
	
	dst->w = sx;
	dst->h = sy;
	
	uint32_t * ptr = dst->data;
	for (int y = 0; y < sy; ++y, ptr += sx)
		dst->line[y] = ptr;
	
	dst->cl = 0;
	dst->ct = 0;
	dst->cr = sx - 1;
	dst->cb = sy - 1;
	
	return dst;
}

BITMAP * create_sub_bitmap(BITMAP * src, int x, int y, int sx, int sy)
{
	BITMAP * dst = new BITMAP();
	
	dst->w = sx;
	dst->h = sy;
	
	uint32_t * ptr = src->data + y * src->w + x;
	for (int y = 0; y < sy; ++y, ptr += src->w)
		dst->line[y] = ptr;
	
	dst->cl = 0;
	dst->ct = 0;
	dst->cr = sx - 1;
	dst->cb = sy - 1;
	
	return dst;
}
	
void destroy_bitmap(BITMAP * bmp)
{
	delete [] bmp->data;
	bmp->data = nullptr;
	
	delete bmp;
	bmp = nullptr;
}

BITMAP * load_bitmap(const char * path, PALETTE pal)
{
	ImageData * image = loadImage(path);
	
	if (image == nullptr)
		return nullptr;
	
	BITMAP * dst = create_bitmap(image->sx, image->sy);
	
	for (int y = 0; y < dst->h; ++y)
	{
		auto * src_line = image->getLine(y);
		auto * dst_line = bmp_write_line(dst, y);
		
		for (int x = 0; x < dst->w; ++x)
		{
			*dst_line = makeacol(src_line->r, src_line->g, src_line->b, src_line->a);
			
			src_line++;
			dst_line++;
		}
	}
	
	return dst;
}

int save_bitmap(const char * path, BITMAP * bmp, PALETTE pal)
{
// todo : save bmp
	return 0;
}

void set_clip(BITMAP * bmp, int cl, int ct, int cr, int cb)
{
	bmp->cl = cl;
	bmp->ct = ct;
	bmp->cr = cr;
	bmp->cb = cb;
}

int makecol(int r, int g, int b)
{
	return
		(r << 0) |
		(g << 8) |
		(b << 16);
}

uint32_t makeacol(int r, int g, int b, int a)
{
	return
		(uint32_t(r) << 0) |
		(uint32_t(g) << 8) |
		(uint32_t(b) << 16) |
		(uint32_t(a) << 24);
}

int getpixel(BITMAP * bmp, int x, int y)
{
	return bmp->line[y][x];
}

void clear(BITMAP * bmp)
{
	for (int y = 0; y < bmp->h; ++y)
	{
		auto * line = bmp_write_line(bmp, y);
		
		for (int x = 0; x < bmp->w; ++x)
		{
			line[x] = 0;
		}
	}
}

void blit(BITMAP * src, BITMAP * dst, int src_x, int src_y, int dst_x, int dst_y, int sx, int sy)
{
	for (int y = 0; y < sy; ++y)
	{
		if (dst_y + y < dst->ct || dst_y + y > dst->cb)
			continue;
			
		uint32_t * src_line = src->line[src_y + y] + src_x;
		uint32_t * dst_line = dst->line[dst_y + y] + dst_x;
		
		for (int x = 0; x < sx; ++x)
		{
			if (dst_x + x < dst->cl || dst_x + x > dst->cr)
				continue;
			
			dst_line[x] = src_line[x];
		}
	}
}

void draw_sprite(BITMAP * dst, BITMAP * src, int x, int y)
{
	blit(src, dst, 0, 0, x, y, src->w, src->h);
}

void draw_lit_sprite(BITMAP * dst, BITMAP * src, int x, int y, int c)
{
	// todo : draw sprite
	
	blit(src, dst, 0, 0, x, y, src->w, src->h);
}

void draw_gouraud_sprite(BITMAP * dst, BITMAP * src, int x, int y, int l1, int l2, int l3, int l4)
{
	// todo : draw sprite
	
	blit(src, dst, 0, 0, x, y, src->w, src->h);
}

void circle(BITMAP * bmp, int x, int y, int r, int c)
{
	// todo : draw circle
	
	rect(bmp, x - r, y - r, x + r, y + r, c);
}

void rect(BITMAP * dst, int x1, int y1, int x2, int y2, int c)
{
	// todo : draw rect
	
	for (int y = y1; y <= y2; ++y)
	{
		if (y < dst->ct || y > dst->cb)
			continue;
			
		uint32_t * dst_line = dst->line[y];
		
		for (int x = x1; x <= x2; ++x)
		{
			if (x < dst->cl || x > dst->cr)
				continue;
			
			if (x == x1 || x == x2 || y == y1 || y == y2)
			{
				dst_line[x] = c;
			}
		}
	}
}

void rectfill(BITMAP * dst, int x1, int y1, int x2, int y2, int c)
{
	for (int y = y1; y <= y2; ++y)
	{
		if (y < dst->ct || y > dst->cb)
			continue;
			
		uint32_t * dst_line = dst->line[y];
		
		for (int x = x1; x <= x2; ++x)
		{
			if (x < dst->cl || x > dst->cr)
				continue;
			
			dst_line[x] = c;
		}
	}
}

void line(BITMAP * bmp, int x1, int y1, int x2, int y2, int c)
{
	// todo : draw line
}

uint32_t * bmp_write_line(BITMAP * bmp, int y)
{
	return bmp->line[y];
}

void bmp_write(uint32_t * addr, int c)
{
	*addr = c;
}

void rgb_to_hsv(int r, int g, int b, float * h, float * s, float * v)
{
	Color color(r, g, b);
	
	color.toHSL(*h, *s, *v);
}
	
void hsv_to_rgb(float h, float s, float v, int * r, int * g, int * b)
{
	Color color = Color::fromHSL(h, s, v);
	
	*r = color.r * 255.f;
	*g = color.g * 255.f;
	*b = color.b * 255.f;
}

void get_palette(PALETTE pal)
{
	// todo : palette
}

void set_palette(PALETTE pal)
{
	// todo : palette
}

void set_palette_range(PALETTE pal, int begin, int end, int vsync)
{
	// todo : palette
}
