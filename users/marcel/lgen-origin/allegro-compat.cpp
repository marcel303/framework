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
	ImageData * image = new ImageData();
	image->imageData = new ImageData::Pixel[bmp->w * bmp->h];
	image->sx = bmp->w;
	image->sy = bmp->h;
	
	for (int y = 0; y < bmp->h; ++y)
		memcpy(image->getLine(y), bmp->line[y], bmp->w * sizeof(ImageData::Pixel));
		
	bool result = saveImage(image, path);
	
	delete image;
	image = nullptr;
	
	return result ? 0 : -1;
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
		(b << 16) |
		(0xff << 24);
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

void draw_sprite(BITMAP * dst, BITMAP * src, int dst_x, int dst_y)
{
	int sx = src->w;
	int sy = src->h;
	
	uint32_t mask = makecol(255, 0, 255);
	
	for (int y = 0; y < sy; ++y)
	{
		if (dst_y + y < dst->ct || dst_y + y > dst->cb)
			continue;
			
		uint32_t * src_line = src->line[        y];
		uint32_t * dst_line = dst->line[dst_y + y] + dst_x;
		
		for (int x = 0; x < sx; ++x)
		{
			if (dst_x + x < dst->cl || dst_x + x > dst->cr)
				continue;
			
			if (src_line[x] == mask)
				continue;
				
			dst_line[x] = src_line[x];
		}
	}
}

static inline int C(int c, int l)
{
	uint8_t v1 = (c >> 0);
	uint8_t v2 = (c >> 8);
	uint8_t v3 = (c >> 16);
	uint8_t v4 = (c >> 24);
	
	v1 = (uint16_t(v1) * l) >> 8;
	v2 = (uint16_t(v2) * l) >> 8;
	v3 = (uint16_t(v3) * l) >> 8;
	
	return
		(uint32_t(v1) << 0) |
		(uint32_t(v2) << 8) |
		(uint32_t(v3) << 16) |
		(uint32_t(v4) << 24);
}

void draw_lit_sprite(BITMAP * dst, BITMAP * src, int dst_x, int dst_y, int c)
{
	int sx = src->w;
	int sy = src->h;
	
	uint32_t mask = makecol(255, 0, 255);
	
	for (int y = 0; y < sy; ++y)
	{
		if (dst_y + y < dst->ct || dst_y + y > dst->cb)
			continue;
			
		uint32_t * src_line = src->line[        y];
		uint32_t * dst_line = dst->line[dst_y + y] + dst_x;
		
		for (int x = 0; x < sx; ++x)
		{
			if (dst_x + x < dst->cl || dst_x + x > dst->cr)
				continue;
			
			if (src_line[x] == mask)
				continue;
				
			dst_line[x] = C(src_line[x], c);
		}
	}
}

void draw_gouraud_sprite(BITMAP * dst, BITMAP * src, int dst_x, int dst_y, int l1, int l2, int l3, int l4)
{
	int sx = src->w;
	int sy = src->h;
	
	uint32_t mask = makecol(255, 0, 255);
	
	for (int y = 0; y < sy; ++y)
	{
		if (dst_y + y < dst->ct || dst_y + y > dst->cb)
			continue;
		
		int c1 = (l1 * (sy - y) + l3 * y) / sy;
		int c2 = (l2 * (sy - y) + l4 * y) / sy;
		
		uint32_t * src_line = src->line[        y];
		uint32_t * dst_line = dst->line[dst_y + y] + dst_x;
		
		for (int x = 0; x < sx; ++x)
		{
			if (dst_x + x < dst->cl || dst_x + x > dst->cr)
				continue;
			
			if (src_line[x] == mask)
				continue;
			
			int c = (c1 * (sx - x) + c2 * x) / sx;
			
			dst_line[x] = C(src_line[x], c);
		}
	}
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
