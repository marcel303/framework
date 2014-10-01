/* Keen Dreams Source Code
 * Copyright (C) 2014 Javier M. Chavez
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

// ID_VW.C

#include "ID_HEADS.H"
#include "syscode.h"

/*
=============================================================================

						 LOCAL CONSTANTS

=============================================================================
*/

#define VIEWWIDTH		40

#define PIXTOBLOCK		4		// 16 pixels to an update block

#if GRMODE == EGAGR
#define SCREENXMASK		(~7)
#define SCREENXPLUS		(7)
#define SCREENXDIV		(8)
#endif

/*
=============================================================================

						 GLOBAL VARIABLES

=============================================================================
*/

cardtype	videocard;		// set by VW_Startup
grtype		grmode;			// CGAgr, EGAgr, VGAgr

unsigned short	bufferofs;		// hidden area to draw to before displaying
unsigned short	displayofs;		// origin of the visable screen
unsigned short	panx,pany;		// panning adjustments inside port in pixels
unsigned short	pansx,pansy;	// panning adjustments inside port in screen
								// block limited pixel values (ie 0/8 for ega x)
unsigned short	panadjust;		// panx/pany adjusted by screen resolution

unsigned short	linewidth;
unsigned short	ylookup[VIRTUALHEIGHT];

boolean		screenfaded;

pictabletype	_seg *pictable;
pictabletype	_seg *picmtable;
spritetabletype _seg *spritetable;

/*
=============================================================================

						 LOCAL VARIABLES

=============================================================================
*/

void	VWL_MeasureString (char far *string, word *width, word *height,
		fontstruct _seg *font);
void 	VWL_DrawCursor (void);
void 	VWL_EraseCursor (void);
void 	VWL_DBSetup (void);
void	VWL_UpdateScreenBlocks (void);


short			bordercolor;
short			cursorvisible;
short			cursornumber,cursorwidth,cursorheight,cursorx,cursory;
memptr			cursorsave;
unsigned short	cursorspot;

extern	unsigned short	bufferwidth,bufferheight;	// used by font drawing stuff

//===========================================================================


/*
=======================
=
= VW_Startup
=
=======================
*/

static	char *ParmStrings[] = {"HIDDENCARD",""};

void	VW_Startup (void)
{
	videocard = EGAcard;

#if GRMODE == EGAGR
	grmode = EGAGR;
	if (videocard != EGAcard && videocard != VGAcard)
		Quit ("Improper video card!  If you really have an EGA/VGA card that I am not \n"
			"detecting, use the -HIDDENCARD command line parameter!");
#endif

	cursorvisible = 0;
}

//===========================================================================

/*
=======================
=
= VW_Shutdown
=
=======================
*/

void	VW_Shutdown (void)
{
	VW_SetScreenMode (TEXTGR);
#if GRMODE == EGAGR
	VW_SetLineWidth (80);
#endif
}

//===========================================================================

/*
========================
=
= VW_SetScreenMode
= Call BIOS to set TEXT / CGAgr / EGAgr / VGAgr
=
========================
*/

void VW_SetScreenMode (short grmode)
{
	VW_SetLineWidth(SCREENWIDTH);
}

/*
=============================================================================

							SCREEN FADES

=============================================================================
*/

char colors[7][17]=
{{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
 {0,0,0,0,0,0,0,0,0,1,2,3,4,5,6,7,0},
 {0,0,0,0,0,0,0,0,0x18,0x19,0x1a,0x1b,0x1c,0x1d,0x1e,0x1f,0},
 {0,1,2,3,4,5,6,7,0x18,0x19,0x1a,0x1b,0x1c,0x1d,0x1e,0x1f,0},
 {0,1,2,3,4,5,6,7,0x1f,0x1f,0x1f,0x1f,0x1f,0x1f,0x1f,0x1f,0},
 {0x1f,0x1f,0x1f,0x1f,0x1f,0x1f,0x1f,0x1f,0x1f,0x1f,0x1f,0x1f,0x1f,0x1f,0x1f,0x1f,0x1f}};


void VW_ColorBorder (short color)
{
	/* mstodo : video : VW_ColorBorder
	_AH=0x10;
	_AL=1;
	_BH=color;
	geninterrupt (0x10);
	*/
	bordercolor = color;
}

void VW_SetDefaultColors(void)
{
#if GRMODE == EGAGR
	colors[3][16] = (char)bordercolor;
	SYS_SetPalette(colors[3]);
#endif
	screenfaded = false;
}


void VW_FadeOut(void)
{
#if GRMODE == EGAGR
	int i;

	for (i=3;i>=0;i--)
	{
	  colors[i][16] = (char)bordercolor;
	  SYS_SetPalette(colors[i]);
	  VW_WaitVBL(6);
	}
#endif
	screenfaded = true;
}


void VW_FadeIn(void)
{
#if GRMODE == EGAGR
	int i;

	for (i=0;i<4;i++)
	{
	  colors[i][16] = (char)bordercolor;
      SYS_SetPalette(colors[i]);
	  VW_WaitVBL(6);
	}
#endif
	screenfaded = false;
}

void VW_FadeUp(void)
{
#if GRMODE == EGAGR
	int i;

	for (i=3;i<6;i++)
	{
	  colors[i][16] = (char)bordercolor;
      SYS_SetPalette(colors[i]);
	  VW_WaitVBL(6);
	}
#endif
	screenfaded = true;
}

void VW_FadeDown(void)
{
#if GRMODE == EGAGR
	int i;

	for (i=5;i>2;i--)
	{
	  colors[i][16] = (char)bordercolor;
	  SYS_SetPalette(colors[i]);
	  VW_WaitVBL(6);
	}
#endif
	screenfaded = false;
}



//===========================================================================

/*
====================
=
= VW_SetLineWidth
=
= Must be an even number of bytes
=
====================
*/

void VW_SetLineWidth (short width)
{
	short i,offset;

//
// set up lookup tables
//
  linewidth = width;

  offset = 0;

  for (i=0;i<VIRTUALHEIGHT;i++)
  {
	ylookup[i]=offset;
	offset += width;
  }
}

//===========================================================================

/*
====================
=
= VW_ClearVideo
=
====================
*/

void	VW_ClearVideo (short color)
{
	unsigned char plane;

	for (plane = 0; plane < 4; ++plane)
		memset(g0xA000[plane], color & (1 << plane) ? 0xff : 0x00, 0xffff); // sizeof(g0xA000[0])
}

void VW_WaitVBL (short number)
{
	while (number--)
	{
		SYS_Present();
	}
}

//===========================================================================

#if NUMPICS>0

/*
====================
=
= VW_DrawPic
=
= X in bytes, y in pixels, chunknum is the #defined picnum
=
====================
*/

void VW_DrawPic(unsigned short x, unsigned short y, unsigned short chunknum)
{
	short	picnum = chunknum - STARTPICS;
	memptr source;
	unsigned short dest,width,height;

	source = grsegs[chunknum];
	dest = ylookup[y]+x+bufferofs;
	width = pictable[picnum].width;
	height = pictable[picnum].height;

	VW_MemToScreen(source,dest,width,height);
}

#endif

#if NUMPICM>0

/*
====================
=
= VW_DrawMPic
=
= X in bytes, y in pixels, chunknum is the #defined picnum
=
====================
*/

void VW_DrawMPic(unsigned short x, unsigned short y, unsigned short chunknum)
{
	short	picnum = chunknum - STARTPICM;
	memptr source;
	unsigned short dest,width,height;

	source = grsegs[chunknum];
	dest = ylookup[y]+x+bufferofs;
	width = pictable[picnum].width;
	height = pictable[picnum].height;

	VW_MaskBlock(source,0,dest,width,height,width*height);
}

#endif

//===========================================================================

#if NUMSPRITES>0

/*
====================
=
= VW_DrawSprite
=
= X and Y in pixels, it will match the closest shift possible
=
= To do:
= Add vertical clipping!
= Make the shifts act as center points, rather than break points
=
====================
*/

void VW_DrawSprite(short x, short y, unsigned short chunknum)
{
	spritetabletype far *spr;
	spritetype _seg	*block;
	unsigned short	dest,shift;

	spr = &spritetable[chunknum-STARTSPRITES];
	block = (spritetype _seg *)grsegs[chunknum];

	y+=spr->orgy>>G_P_SHIFT;
	x+=spr->orgx>>G_P_SHIFT;

#if GRMODE == EGAGR
#if SUPER_SMOOTH_SCROLLING
	shift = (x&7)
#else
	shift = (x&7)/2;
#endif
#endif

	dest = bufferofs + ylookup[y];
	if (x>=0)
		dest += x/SCREENXDIV;
	else
		dest += (x+1)/SCREENXDIV;

	VW_MaskBlock (block,block->sourceoffset[shift],dest,
		block->width[shift],spr->height,block->planesize[shift]);
}

#endif


/*
==================
=
= VW_Hlin
=
==================
*/


#if GRMODE == EGAGR

unsigned char leftmask[8] = {0xff,0x7f,0x3f,0x1f,0xf,7,3,1};
unsigned char rightmask[8] = {0x80,0xc0,0xe0,0xf0,0xf8,0xfc,0xfe,0xff};

void VW_Hlin(unsigned short xl, unsigned short xh, unsigned short y, unsigned short color)
{
	unsigned short dest,xlb,xhb,maskleft,maskright,mid,plane,i;

	xlb=xl/8;
	xhb=xh/8;

	maskleft = leftmask[xl&7];
	maskright = rightmask[xh&7];

	mid = xhb-xlb-1;
	dest = bufferofs+ylookup[y]+xlb;

	if (xlb==xhb)
	{
		//
		// entire line is in one byte
		//

		maskleft&=maskright;

		for (plane = 0; plane < 4; ++plane)
		{
			unsigned char * __restrict dst = g0xA000[plane] + dest;
			dst[0] = (dst[0] & ~maskleft) | (color & maskleft);
		}
	}
	else
	{
		for (plane = 0; plane < 4; ++plane)
		{
			unsigned char * __restrict dst = g0xA000[plane] + dest;
			unsigned char pcolor = (color & (1 << plane)) ? 0xff : 0x00;

			// draw left side
			*dst++ = (*dst & ~maskleft) | (pcolor & maskleft);

			// draw middle
			for (i = 0; i < mid; ++i)
				*dst++ = pcolor;

			// draw right side
			*dst++ = (*dst & ~maskright) | (pcolor & maskright);
		}
	}
}
#endif


/*
==================
=
= VW_Bar
=
= Pixel addressable block fill routine
=
==================
*/


#if	GRMODE == EGAGR

void VW_Bar (unsigned short x, unsigned short y, unsigned short width, unsigned short height, unsigned short color)
{
	unsigned short maskleft,maskright,xh,xlb,xhb,mid,plane,i;

	xh = x+width-1;
	xlb=x/8;
	xhb=xh/8;

	maskleft = leftmask[x&7];
	maskright = rightmask[xh&7];

	mid = xhb-xlb-1;

	for (plane = 0; plane < 4; ++plane)
	{
		unsigned char pcolor = (color & (1 << plane)) ? 0xff : 0x00;
		unsigned short iy;

		for (iy = 0; iy < height; ++iy)
		{
			unsigned char * dst = &g0xA000[plane][bufferofs + ylookup[y + iy] + x / 8];

			if (xlb == xhb)
			{
				maskleft &= maskright;

				*dst++ = (*dst & ~maskleft) | (pcolor & maskleft);
			}
			else
			{
				// draw left side
				*dst++ = (*dst & ~maskleft) | (pcolor & maskleft);

				// draw middle
				for (i = 0; i < mid; ++i)
					*dst++ = pcolor;

				// draw right side
				*dst++ = (*dst & ~maskright) | (pcolor & maskright);
			}
		}
	}
}

#endif

//==========================================================================

/*
==================
=
= VW_MeasureString
=
==================
*/

#if NUMFONT+NUMFONTM>0
void
VWL_MeasureString (char far *string, word *width, word *height, fontstruct _seg *font)
{
	*height = font->height;
	for (*width = 0;*string;string++)
		*width += font->width[(unsigned char)(*string)];		// proportional width
}

void	VW_MeasurePropString (char far *string, word *width, word *height)
{
	VWL_MeasureString(string,width,height,(fontstruct _seg *)grsegs[STARTFONT]);
}

void	VW_MeasureMPropString  (char far *string, word *width, word *height)
{
	VWL_MeasureString(string,width,height,(fontstruct _seg *)grsegs[STARTFONTM]);
}


#endif


/*
=============================================================================

					   CURSOR ROUTINES

These only work in the context of the double buffered update routines

=============================================================================
*/

/*
====================
=
= VWL_DrawCursor
=
= Background saves, then draws the cursor at cursorspot
=
====================
*/

void VWL_DrawCursor (void)
{
	cursorspot = bufferofs + ylookup[cursory+pansy]+(cursorx+pansx)/SCREENXDIV;
	VW_ScreenToMem(cursorspot,cursorsave,cursorwidth,cursorheight);
	VWB_DrawSprite(cursorx,cursory,cursornumber);
}


//==========================================================================


/*
====================
=
= VWL_EraseCursor
=
====================
*/

void VWL_EraseCursor (void)
{
	VW_MemToScreen(cursorsave,cursorspot,cursorwidth,cursorheight);
	VW_MarkUpdateBlock ((cursorx+pansx)&SCREENXMASK,cursory+pansy,
		( (cursorx+pansx)&SCREENXMASK)+cursorwidth*SCREENXDIV-1,
		cursory+pansy+cursorheight-1);
}


//==========================================================================


/*
====================
=
= VW_ShowCursor
=
====================
*/

void VW_ShowCursor (void)
{
	cursorvisible++;
}


//==========================================================================

/*
====================
=
= VW_HideCursor
=
====================
*/

void VW_HideCursor (void)
{
	cursorvisible--;
}

//==========================================================================

/*
====================
=
= VW_MoveCursor
=
====================
*/

void VW_MoveCursor (short x, short y)
{
	cursorx = x;
	cursory = y;
}

//==========================================================================

/*
====================
=
= VW_SetCursor
=
= Load in a sprite to be used as a cursor, and allocate background save space
=
====================
*/

void VW_SetCursor (short spritenum)
{
	if (cursornumber)
	{
		MM_SetLock (&grsegs[cursornumber],false);
		MM_FreePtr (&cursorsave);
	}

	cursornumber = spritenum;

	CA_CacheGrChunk (spritenum);
	MM_SetLock (&grsegs[cursornumber],true);

	cursorwidth = spritetable[spritenum-STARTSPRITES].width+1;
	cursorheight = spritetable[spritenum-STARTSPRITES].height;

	MM_GetPtr (&cursorsave,cursorwidth*cursorheight*5);
}


/*
=============================================================================

				Double buffer management routines

=============================================================================
*/

/*
======================
=
= VW_InitDoubleBuffer
=
======================
*/

void VW_InitDoubleBuffer (void)
{
#if GRMODE == EGAGR
	VW_SetScreen (displayofs+panadjust,0);			// no pel pan
#endif
}


/*
======================
=
= VW_FixRefreshBuffer
=
= Copies the view page to the buffer page on page flipped refreshes to
= avoid a one frame shear around pop up windows
=
======================
*/

void VW_FixRefreshBuffer (void)
{
#if GRMODE == EGAGR
	VW_ScreenToScreen (displayofs,bufferofs,PORTTILESWIDE*4*CHARWIDTH,
		PORTTILESHIGH*16);
#endif
}


/*
======================
=
= VW_QuitDoubleBuffer
=
======================
*/

void VW_QuitDoubleBuffer (void)
{
}


/*
=======================
=
= VW_MarkUpdateBlock
=
= Takes a pixel bounded block and marks the tiles in bufferblocks
= Returns 0 if the entire block is off the buffer screen
=
=======================
*/

int VW_MarkUpdateBlock (short x1, short y1, short x2, short y2)
{
	short x,y,xt1,yt1,xt2,yt2,nextline;
	byte *mark;

	xt1 = x1>>PIXTOBLOCK;
	yt1 = y1>>PIXTOBLOCK;

	xt2 = x2>>PIXTOBLOCK;
	yt2 = y2>>PIXTOBLOCK;

	if (xt1<0)
		xt1=0;
	else if (xt1>=UPDATEWIDE-1)
		return 0;

	if (yt1<0)
		yt1=0;
	else if (yt1>UPDATEHIGH)
		return 0;

	if (xt2<0)
		return 0;
	else if (xt2>=UPDATEWIDE-1)
		xt2 = UPDATEWIDE-2;

	if (yt2<0)
		return 0;
	else if (yt2>=UPDATEHIGH)
		yt2 = UPDATEHIGH-1;

	mark = updateptr + uwidthtable[yt1] + xt1;
	nextline = UPDATEWIDE - (xt2-xt1) - 1;

	for (y=yt1;y<=yt2;y++)
	{
		for (x=xt1;x<=xt2;x++)
			*mark++ = 1;			// this tile will need to be updated

		mark += nextline;
	}

	return 1;
}


/*
===========================
=
= VW_UpdateScreen
=
= Updates any changed areas of the double buffer and displays the cursor
=
===========================
*/

void VW_UpdateScreen (void)
{
	if (cursorvisible>0)
		VWL_DrawCursor();

#if GRMODE == EGAGR
	VWL_UpdateScreenBlocks();

	VW_SetScreen(displayofs+panadjust,0);
#endif

	if (cursorvisible>0)
		VWL_EraseCursor();
}



void VWB_DrawTile8 (short x, short y, short tile)
{
	x+=pansx;
	y+=pansy;
	if (VW_MarkUpdateBlock (x&SCREENXMASK,y,(x&SCREENXMASK)+7,y+7))
		VW_DrawTile8 (x/SCREENXDIV,y,tile);
}

void VWB_DrawTile8M (short x, short y, short tile)
{
	short xb;

	x+=pansx;
	y+=pansy;
	xb = x/SCREENXDIV; 			// use intermediate because VW_DT8M is macro
	if (VW_MarkUpdateBlock (x&SCREENXMASK,y,(x&SCREENXMASK)+7,y+7))
		VW_DrawTile8M (xb,y,tile);
}

void VWB_DrawTile16 (short x, short y, short tile)
{
	x+=pansx;
	y+=pansy;
	if (VW_MarkUpdateBlock (x&SCREENXMASK,y,(x&SCREENXMASK)+15,y+15))
		VW_DrawTile16 (x/SCREENXDIV,y,tile);
}

void VWB_DrawTile16M (short x, short y, short tile)
{
	short xb;

	x+=pansx;
	y+=pansy;
	xb = x/SCREENXDIV;		// use intermediate because VW_DT16M is macro
	if (VW_MarkUpdateBlock (x&SCREENXMASK,y,(x&SCREENXMASK)+15,y+15))
		VW_DrawTile16M (xb,y,tile);
}

#if NUMPICS
void VWB_DrawPic (short x, short y, short chunknum)
{
// mostly copied from drawpic
	short	picnum = chunknum - STARTPICS;
	memptr source;
	unsigned short dest,width,height;

	x+=pansx;
	y+=pansy;
	x/= SCREENXDIV;

	source = grsegs[chunknum];
	dest = ylookup[y]+x+bufferofs;
	width = pictable[picnum].width;
	height = pictable[picnum].height;

	if (VW_MarkUpdateBlock (x*SCREENXDIV,y,(x+width)*SCREENXDIV-1,y+height-1))
		VW_MemToScreen(source,dest,width,height);
}
#endif

#if NUMPICM>0
void VWB_DrawMPic(short x, short y, short chunknum)
{
// mostly copied from drawmpic
	short	picnum = chunknum - STARTPICM;
	memptr source;
	unsigned short dest,width,height;

	x+=pansx;
	y+=pansy;
	x/=SCREENXDIV;

	source = grsegs[chunknum];
	dest = ylookup[y]+x+bufferofs;
	width = picmtable[picnum].width;
	height = picmtable[picnum].height;

	if (VW_MarkUpdateBlock (x*SCREENXDIV,y,(x+width)*SCREENXDIV-1,y+height-1))
		VW_MaskBlock(source,0,dest,width,height,width*height);
}
#endif


void VWB_Bar (short x, short y, short width, short height, short color)
{
	x+=pansx;
	y+=pansy;
	if (VW_MarkUpdateBlock (x,y,x+width,y+height-1) )
		VW_Bar (x,y,width,height,color);
}


#if NUMFONT
void VWB_DrawPropString	 (char far *string)
{
	short x,y;
	x = px+pansx;
	y = py+pansy;
	VW_DrawPropString (string);
//	VW_MarkUpdateBlock(0,0,320,200);
	VW_MarkUpdateBlock(x,y,x+bufferwidth*8-1,y+bufferheight-1);
}
#endif


#if NUMFONTM
void VWB_DrawMPropString (char far *string)
{
	short x,y;
	x = px+pansx;
	y = py+pansy;
	VW_DrawMPropString (string);
	VW_MarkUpdateBlock(x,y,x+bufferwidth*8-1,y+bufferheight-1);
}
#endif

#if NUMSPRITES
void VWB_DrawSprite(short x, short y, short chunknum)
{
	spritetabletype far *spr;
	spritetype _seg	*block;
	unsigned short	dest,shift,width,height;

	x+=pansx;
	y+=pansy;

	spr = &spritetable[chunknum-STARTSPRITES];
	block = (spritetype _seg *)grsegs[chunknum];

	y+=spr->orgy>>G_P_SHIFT;
	x+=spr->orgx>>G_P_SHIFT;


#if GRMODE == EGAGR
#if SUPER_SMOOTH_SCROLLING
	shift = (x&7);
#else
	shift = (x&7)/2;
#endif
#endif

	dest = bufferofs + ylookup[y];
	if (x>=0)
		dest += x/SCREENXDIV;
	else
		dest += (x+1)/SCREENXDIV;

	width = block->width[shift];
	height = spr->height;

	if (VW_MarkUpdateBlock (x&SCREENXMASK,y,(x&SCREENXMASK)+width*SCREENXDIV-1
		,y+height-1))
		VW_MaskBlock (block,block->sourceoffset[shift],dest,
			width,height,block->planesize[shift]);
}
#endif

/*
void VWB_Plot (short x, short y, short color)
{
	x+=pansx;
	y+=pansy;
	if (VW_MarkUpdateBlock (x,y,x,y))
		VW_Plot(x,y,color);
}
*/

void VWB_Hlin (short x1, short x2, short y, short color)
{
	x1+=pansx;
	x2+=pansx;
	y+=pansy;
	if (VW_MarkUpdateBlock (x1,y,x2,y))
		VW_Hlin(x1,x2,y,color);
}

void VWB_Vlin (short y1, short y2, short x, short color)
{
	x+=pansx;
	y1+=pansy;
	y2+=pansy;
	if (VW_MarkUpdateBlock (x,y1,x,y2))
		VW_Vlin(y1,y2,x,color);
}


//===========================================================================
