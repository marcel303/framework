#include "ID_HEADS.H"
#include "syscode.h"

//

extern byte				*updatestart[2];
extern unsigned short	originmap;
extern unsigned short	updatemapofs[UPDATEWIDE*UPDATEHIGH];
extern unsigned short	tilecache[NUMTILE16];

/*
RFL_NewTile

Draws a composit two plane tile to the master screen and sets the update
spot to 1 in both update pages, forcing the tile to be copied to the
view pages the next two refreshes

Called to draw newlly scrolled on strips and animating tiles
*/
void RFL_NewTile (unsigned short updateoffset)
{
	// msnote : verification ok!

	unsigned short screenstart = (blockstarts[updateoffset] + masterofs);
	unsigned short mapIndex = (updatemapofs[updateoffset] + originmap) >> 1;
	unsigned short foregroundTile = mapsegs[1][mapIndex];
	unsigned short backgroundTile = mapsegs[0][mapIndex];

	updatestart[0][updateoffset] = 1;
	updatestart[1][updateoffset] = 1;

	if (foregroundTile)
	{
		// foregroundTile + backgroundTile. draw using mask

		unsigned char * __restrict src1 = (unsigned char*)grsegs[STARTTILE16  + backgroundTile];
		unsigned char * __restrict src2 = (unsigned char*)grsegs[STARTTILE16M + foregroundTile] + 32;
		unsigned char plane;

		for (plane = 0; plane < 4; ++plane)
		{
			unsigned char * __restrict mask = (unsigned char*)grsegs[STARTTILE16M + foregroundTile];
			unsigned char * __restrict dest = &g0xA000[plane][screenstart];
			unsigned char y, i;

			for (y = 0; y < 16; ++y)
			{
				for (i = 0; i < 2; ++i)
				{
					dest[i]  = src1[i];
					dest[i] &= mask[i];
					dest[i] |= src2[i];
				}

				src1 += 2;
				src2 += 2;
				mask += 2;
				dest += SCREENWIDTH;
			}
		}
	}
	else
	{
		// backgroundTile only. draw without masking

		unsigned char * __restrict src = (unsigned char*)grsegs[STARTTILE16 + backgroundTile];
		unsigned char plane;

		tilecache[backgroundTile] = screenstart;

		for (plane = 0; plane < 4; ++plane)
		{
			unsigned char * __restrict dst = &g0xA000[plane][screenstart];
			unsigned char y, i;

			for (y = 0; y < 16; ++y)
			{
				for (i = 0; i < 2; ++i)
					dst[i] = src[i];

				src += 2;
				dst += SCREENWIDTH;
			}
		}
	}
}

/*
RFL_UpdateTiles

Scans through the update matrix pointed to by updateptr, looking for 1s.
A 1 represents a tile that needs to be copied from the master screen to the
current screen (a new row or an animated tiled)
*/
void RFL_UpdateTiles (void)
{
	// msnote : verification ok!

	unsigned char * p = updateptr;
	unsigned char * pend = updateptr + (PORTTILESWIDE+1)*PORTTILESHIGH + 1;

	for (;;)
	{
		while (*p++ != 1)
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
				unsigned char * __restrict src = &g0xA000[plane][blockofs + masterofs];
				unsigned char * __restrict dst = &g0xA000[plane][blockofs + bufferofs];
				unsigned char i;

				for (i = 0; i < 16; ++i)
				{
					dst[0] = src[0];
					dst[1] = src[1];

					src += SCREENWIDTH;
					dst += SCREENWIDTH;
				}
			}
		}
	}
}

/*
RFL_MaskForegroundTiles

Scan through update looking for 3's.  If the foreground tile there is a
masked foreground tile, draw it to the screen
*/
void RFL_MaskForegroundTiles (void)
{
	unsigned char * __restrict p = updateptr;
	unsigned char * __restrict pend = updateptr + (PORTTILESWIDE+1)*PORTTILESHIGH + 2;

	// msnote : verification unclear

	for (;;)
	{
		while (*p++ != 3)
		{
			// loop
		}

		if (p == pend)
			break;
		else
		{
			unsigned short pidx;
			unsigned short mapIndex;
			unsigned short tileIndex;
			unsigned char * __restrict src;
			unsigned char plane;

			pidx = p - 1 - updateptr;

			mapIndex = (updatemapofs[pidx] + originmap) >> 1;
			tileIndex = mapsegs[1][mapIndex];

			if (tileIndex == 0)
			{
				// no foreground tile

				continue;
			}

			if ((tinf[INTILE + tileIndex] & 0x80) == 0)
			{
				// high bit = masked tile

				continue;
			}

			// mask the tile

			src = (unsigned char *)grsegs[STARTTILE16M + tileIndex] + 32;

			for (plane = 0; plane < 4; ++plane)
			{
				unsigned short blockofs = blockstarts[pidx];
				unsigned char * __restrict mask = (unsigned char*)grsegs[STARTTILE16M + tileIndex];
				unsigned char * __restrict dest = &g0xA000[plane][blockofs + bufferofs];
				unsigned char y, i;

				for (y = 0; y < 16; ++y)
				{
					for (i = 0; i < 2; ++i)
					{
						dest[i] &= mask[i];
						dest[i] |= src[i];
					}

					src += 2;
					mask += 2;
					dest += SCREENWIDTH;
				}
			}
		}
	}
}
