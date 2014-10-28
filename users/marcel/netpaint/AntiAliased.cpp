#include <allegro.h>
#include "AntiAliased.h"

#define	_getpixel8(bmp,	x, y) _getpixel(bmp, x,	y)
#define	_putpixel8(bmp,	x, y, c) _putpixel(bmp,	x, y, c)

#define	DECLARE_AA_PUTPIXEL(bpp) \
void aa_putpixel##bpp##(BITMAP*	dst, int x, int	y, int c) { \
 \
	if (x <	0 || y < 0 || (x>>16) >= dst->w-1 || (y>>16) >=	dst->h-1) \
		return;	\
 \
	const int r1 = getr##bpp##(c); \
	const int g1 = getg##bpp##(c); \
	const int b1 = getb##bpp##(c); \
 \
	const int px = x>>16;	\
	const int py = y>>16;	\
 \
	const int sx = (x>>10)&63; \
	const int sy = (y>>10)&63; \
 \
	{ \
		int a =	((63-sx)*(63-sy)<<8)/*/3969*/>>12; \
		int a2 = 255-a;	\
		int dc = _getpixel##bpp##(dst, px, py);	\
		int r2 = (getr##bpp##(dc)*a2+a*r1)>>8; if (r2 >	255) r2	= 255; \
		int g2 = (getg##bpp##(dc)*a2+a*g1)>>8; if (g2 >	255) g2	= 255; \
		int b2 = (getb##bpp##(dc)*a2+a*b1)>>8; if (b2 >	255) b2	= 255; \
		_putpixel##bpp##(dst, px, py, makecol##bpp##(r2, g2, b2)); \
 \
		a = (sx*(63-sy)<<8)/*/3969*/>>12; \
		a2 = 255-a; \
		dc = _getpixel##bpp##(dst, px+1, py); \
		r2 = (getr##bpp##(dc)*a2+a*r1)>>8; if (r2 > 255) r2 = 255; \
		g2 = (getg##bpp##(dc)*a2+a*g1)>>8; if (g2 > 255) g2 = 255; \
		b2 = (getb##bpp##(dc)*a2+a*b1)>>8; if (b2 > 255) b2 = 255; \
		_putpixel##bpp##(dst, px+1, py,	makecol##bpp##(r2, g2, b2)); \
 \
		a = (sx*sy<<8)/*/3969*/>>12; \
		a2 = 255-a; \
		dc = _getpixel##bpp##(dst, px+1, py+1);	\
		r2 = (getr##bpp##(dc)*a2+a*r1)>>8; if (r2 > 255) r2 = 255; \
		g2 = (getg##bpp##(dc)*a2+a*g1)>>8; if (g2 > 255) g2 = 255; \
		b2 = (getb##bpp##(dc)*a2+a*b1)>>8; if (b2 > 255) b2 = 255; \
		_putpixel##bpp##(dst, px+1, py+1, makecol##bpp##(r2, g2, b2)); \
 \
		a = ((63-sx)*sy<<8)/*/3969*/>>12; \
		a2 = 255-a; \
		dc = _getpixel##bpp##(dst, px, py+1); \
		r2 = (getr##bpp##(dc)*a2+a*r1)>>8; if (r2 > 255) r2 = 255; \
		g2 = (getg##bpp##(dc)*a2+a*g1)>>8; if (g2 > 255) g2 = 255; \
		b2 = (getb##bpp##(dc)*a2+a*b1)>>8; if (b2 > 255) b2 = 255; \
		_putpixel##bpp##(dst, px, py+1,	makecol##bpp##(r2, g2, b2)); \
	} \
 \
}

#define	DECLARE_AA_LINE(bpp) \
void aa_line##bpp##(BITMAP* dst, int x1, int y1, int x2, int y2, int c)	{ \
 \
	int dx = x2-x1;	\
	int dy = y2-y1;	\
	int s =	MAX(ABS(fixtoi(dx)), ABS(fixtoi(dy))); \
	int ddx, ddy; \
 \
	if (s) { \
		ddx = dx/s; \
		ddy = dy/s; \
	} else {\
		ddx = 0; \
		ddy = 0; \
	} \
 \
	int x =	x1; \
	int y =	y1; \
 \
	for (int i=0; i<=s; i++) { \
		aa_putpixel##bpp##(dst,	x, y, c); \
		x += ddx; \
		y += ddy; \
	} \
 \
}

DECLARE_AA_PUTPIXEL(8);
DECLARE_AA_PUTPIXEL(15);
DECLARE_AA_PUTPIXEL(16);
DECLARE_AA_PUTPIXEL(24);
DECLARE_AA_PUTPIXEL(32);

DECLARE_AA_LINE(8);
DECLARE_AA_LINE(15);
DECLARE_AA_LINE(16);
DECLARE_AA_LINE(24);
DECLARE_AA_LINE(32);

void aa_putpixel(BITMAP* dst, int x, int y, int	c) {

	int bpp	= bitmap_color_depth(dst);

	switch(bpp) {
		#define	CASE(bpp) \
		case bpp: \
			aa_putpixel##bpp##(dst,	x, y, c); \
		break;
		CASE(8);
		CASE(15);
		CASE(16);
		CASE(24);
		CASE(32);
		#undef CASE
	}

}

void aa_line(BITMAP* dst, int x1, int y1, int x2, int y2, int c) {

	#if 1

        acquire_bitmap(dst);

	int bpp	= bitmap_color_depth(dst);

	switch(bpp) {
		#define	CASE(bpp) \
		case bpp: \
			aa_line##bpp##(dst, x1,	y1, x2,	y2, c);	\
		break;
		CASE(8);
		CASE(15);
		CASE(16);
		CASE(24);
		CASE(32);
		#undef CASE
	}

        release_bitmap(dst);

	#else

	line(dst, x1>>16, y1>>16, x2>>16, y2>>16, c);

	#endif

}
