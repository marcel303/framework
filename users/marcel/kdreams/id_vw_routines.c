#include "ID_HEADS.H"
#include "syscode.h"

// VW_*
short			px,py;
byte			pdrawmode,fontcolor;

unsigned short	bufferwidth,bufferheight;	// used by font drawing stuff

/*
VW_MaskBlock

Draws a masked block shape to the screen.  bufferofs is NOT accounted for.
The mask comes first, then four planes of data.
*/
void VW_MaskBlock(memptr segment, unsigned short ofs, unsigned short destofs, unsigned short wide, unsigned short height, unsigned short planesize)
{
	// msnote : verified ok!

	unsigned char * __restrict mask = (unsigned char*)segment + ofs;
	unsigned char * __restrict cols = mask + planesize;
	unsigned char plane;

	for (plane = 0; plane < 4; ++plane, cols += planesize)
	{
		unsigned char * __restrict dest = g0xA000[plane] + destofs;
		unsigned short idx = 0;
		unsigned short x, y;

		for (y = 0; y < height; ++y)
		{
			for (x = 0; x < wide; ++x)
			{
				unsigned char d = dest[x];
				unsigned char m = mask[idx];
				unsigned char c = cols[idx];

				dest[x] = (d & m) | c;

				idx += 1;
			}

			dest += linewidth;
		}
	}
}

/*
VWL_UpdateScreenBlocks

Scans through the update matrix and copies any areas that have changed
to the visable screen, then zeros the update array
*/
void VWL_UpdateScreenBlocks()
{
	// msnote : verification unclear

	unsigned char * __restrict p = updateptr;
	unsigned char * __restrict pend = updateptr + UPDATEWIDE*UPDATEHIGH + 1;

	for (;;)
	{
		while (*p++ != 1 && p != pend) // mstodo : update array doesn't seem to be terminated with UPDATETERMINATE? at least not the first time we get here..
		{
			// loop
		}

		if (p == pend)
			break;
		else
		{
			unsigned short pidx;
			unsigned char plane;

			pidx = p - 1 - updateptr;

			for (plane = 0; plane < 4; ++plane)
			{
				unsigned short blockofs = blockstarts[pidx];
				unsigned char * __restrict dst = &g0xA000[plane][blockofs + displayofs];
				unsigned char * __restrict src = &g0xA000[plane][blockofs + bufferofs];
				unsigned char y, i;

				for (y = 0; y < 16; ++y)
				{
					for (i = 0; i < 2; ++i)
						dst[i] = src[i];

					src += linewidth;
					dst += linewidth;
				}
			}
		}
	}

	// clear out the update matrix
	memset(updateptr, 0, UPDATEWIDE*UPDATEHIGH);
}

void VW_Plot(unsigned short x, unsigned short y, unsigned short color)
{
	// mstodo : VW_Plot
}

void VW_Vlin(unsigned short yl, unsigned short yh, unsigned short x, unsigned short color)
{
	// mstodo : VW_Vline
}

// font drawing
#define BUFFWIDTH	50
#define BUFFHEIGHT	32 //must be twice as high as font for masked fonts

static unsigned char databuffer[BUFFWIDTH*BUFFHEIGHT];
static unsigned short bufferbyte;
static unsigned short bufferbit;

// offsets in font structure
static const unsigned short pcharheight	=	0		; // lines high
static const unsigned short charloc		=	2		; // pointers to every character
static const unsigned short charwidth		=	514		; // every character's width in pixels

#define SHIFTNOXOR \
	{ \
		const unsigned char value = *src; \
		const unsigned short shifted = shifttable[value]; \
		dst[0] |= (shifted >> 0) & 0xff; \
		dst[1]  = (shifted >> 8) & 0xff; \
		src++; \
		dst++; \
	}

static void ShiftPropChar(char c)
{
	// find character location, width, and height
	const unsigned char * __restrict fontdata = (unsigned char *)grsegs[STARTFONT];
	const unsigned short srcpos = *(unsigned short *)&fontdata[charloc + c * 2];
	const unsigned char * __restrict src = &fontdata[srcpos];
	unsigned char width  = fontdata[charwidth + c];
	unsigned char height = fontdata[pcharheight];

	// write position in databuffer
	unsigned char * __restrict dst = &databuffer[bufferbyte];

	// look up which shift table to use, based on bufferbit
	const unsigned short * shifttable = shifttabletable[bufferbit];

	// advance position by character width
	const unsigned short nextbitposition = bufferbit + width;
	bufferbit = nextbitposition & 7;
	bufferbyte += nextbitposition >> 3;

	width = (width + 7) >> 3;

	while (height--)
	{
		unsigned char i;

		for (i = 0; i < width; ++i)
		{
			SHIFTNOXOR
		}

		dst += BUFFWIDTH - width;
	}
}

/*
VW_DrawPropString

Draws a C string of characters at px/py and advances px
*/
void VW_DrawPropString (char far *string)
{
	// msnote : verification ok!

	unsigned short y;
	unsigned char plane;

	for (y = 0; y < BUFFHEIGHT; ++y)
		databuffer[y * BUFFWIDTH] = 0;

	// shift the characters into the buffer

	bufferbit = px & 7;
	bufferbyte = 0;

	while (*string)
	{
		ShiftPropChar(*string++);
	}

	// draw it

	for (plane = 0; plane < 4; ++plane)
	{
		unsigned char * __restrict src = databuffer;
		unsigned char * __restrict dst = g0xA000[plane] + bufferofs + ylookup[py] + panadjust + (px >> 3);
		unsigned char * __restrict fontdata = (unsigned char *)grsegs[STARTFONT];
		unsigned short y;

		bufferwidth = bufferbyte + (bufferbit ? 1 : 0); // so the partial byte also gets drawn
		bufferheight = fontdata[pcharheight];

		for (y = 0; y < bufferheight; ++y)
		{
			unsigned short x;

			for (x = 0; x < bufferwidth; ++x)
			{
				// XOR based drawing

				dst[x] ^= src[x] & ((fontcolor & (1 << plane)) ? 0xff : 0x00);
			}

			src += BUFFWIDTH;
			dst += linewidth;
		}
	}

	// advance px

	px += (bufferbyte << 3) | bufferbit;
}

/*
VW_DrawTile8

xcoord in bytes (8 pixels), ycoord in pixels
*/
void VW_DrawTile8(unsigned short x, unsigned short y, unsigned short tile)
{
	unsigned char * __restrict src = (unsigned char *)grsegs[STARTTILE8] + (tile << 5);
	unsigned char plane;

	// msnote : verification unclear

	for (plane = 0; plane < 4; ++plane)
	{
		unsigned char * __restrict dst = &g0xA000[plane][bufferofs + ylookup[y] + x];
		unsigned char i;

		for (i = 0; i < 8; ++i)
		{
			*dst = *src;

			src += 1;
			dst += linewidth;
		}
	}
}

/*
VW_MemToScreen

Basic block drawing routine. Takes a block shape at segment pointer source
with four planes of width by height data, and draws it to dest in the
virtual screen, based on linewidth.  bufferofs is NOT accounted for.
*/
void VW_MemToScreen(memptr source, unsigned short dest, unsigned short width, unsigned short height)
{
	// msnote : verified ok!

	unsigned char plane;
	unsigned char * __restrict src = (unsigned char *)source;

	for (plane = 0; plane < 4; ++plane)
	{
		unsigned char * __restrict dst = g0xA000[plane] + dest;
		unsigned short y;

		for (y = 0; y < height; ++y)
		{
			memcpy(dst, src, width);
			src += width;
			dst += linewidth;
		}
	}
}

/*
VW_ScreenToMem

Copies a block of video memory to main memory, in order from planes 0-3.
*/
void VW_ScreenToMem(unsigned short source, memptr dest, unsigned short width, unsigned short height)
{
	// msnote : verified ok!

	unsigned char plane;
	unsigned char * __restrict dst = (unsigned char *)dest;

	for (plane = 0; plane < 4; ++plane)
	{
		unsigned char * __restrict src = g0xA000[plane] + source;
		unsigned short y;

		for (y = 0; y < height; ++y)
		{
			memcpy(dst, src, width);
			src += linewidth;
			dst += width;
		}
	}
}

/*
VW_ScreenToScreen

Basic block copy routine.  Copies one block of screen memory to another,
using write mode 1. bufferofs is NOT accounted for.
*/
void VW_ScreenToScreen(unsigned short source, unsigned short dest, unsigned short width, unsigned short height)
{
	// msnote : verified ok!

	unsigned char plane;

	for (plane = 0; plane < 4; ++plane)
	{
		unsigned char * __restrict src = g0xA000[plane] + source;
		unsigned char * __restrict dst = g0xA000[plane] + dest;
		unsigned short y;

		for (y = 0; y < height; ++y)
		{
			memcpy(dst, src, width);
			src += linewidth;
			dst += linewidth;
		}
	}
}
