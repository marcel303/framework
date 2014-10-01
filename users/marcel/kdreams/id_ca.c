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

// ID_CA.C

/*
=============================================================================

Id Software Caching Manager
---------------------------

Must be started BEFORE the memory manager, because it needs to get the headers
loaded into the data segment

=============================================================================
*/

#include "ID_HEADS.H"
#include "static/AUDIODCT.h"
#include "static/AUDIOHHD.h"
#include "static/EGADICT.h"
#include "static/EGAHEAD.h"
#include "static/MAPDICT.h"
#include "static/MAPHEAD.h"

#pragma hdrstop

/*
=============================================================================

						 LOCAL CONSTANTS

=============================================================================
*/

typedef struct
{
  unsigned short bit0,bit1;	// 0-255 is a character, > is a pointer to a node
} huffnode;

#pragma pack(push)
#pragma pack(2)

typedef struct
{
	unsigned short	RLEWtag;
	long			headeroffsets[100];
	byte			headersize[100];		// headers are very small
	//byte			tileinfo[];
} mapfiletype;

#pragma pack(pop)

/*
=============================================================================

						 GLOBAL VARIABLES

=============================================================================
*/

byte 		_seg	*tinf;
short		mapon;

unsigned short	_seg	*mapsegs[3];
maptype			_seg	*mapheaderseg[NUMMAPS];
byte			_seg	*audiosegs[NUMSNDCHUNKS];
void			_seg	*grsegs[NUMCHUNKS];

byte		grneeded[NUMCHUNKS];
byte		ca_levelbit,ca_levelnum;

char		*titleptr[8];

int			profilehandle = -1;

/*
=============================================================================

						 LOCAL VARIABLES

=============================================================================
*/

long		_seg *grstarts;	// array of offsets in egagraph, -1 for sparse
long		_seg *audiostarts;	// array of offsets in audio / audiot

#ifdef GRHEADERLINKED
huffnode	*grhuffman;
#else
huffnode	grhuffman[255];
#endif

#ifdef MAPHEADERLINKED
huffnode	*maphuffman;
#else
huffnode	maphuffman[255];
#endif

#ifdef AUDIOHEADERLINKED
huffnode	*audiohuffman;
#else
huffnode	audiohuffman[255];
#endif


int			grhandle = -1;		// handle to EGAGRAPH
int			maphandle = -1;		// handle to MAPTEMP / GAMEMAPS
int			audiohandle = -1;	// handle to AUDIOT / AUDIO

long		chunkcomplen,chunkexplen;

SDMode		oldsoundmode;

/*
=============================================================================

					   LOW LEVEL ROUTINES

=============================================================================
*/

/*
============================
=
= CAL_GetGrChunkLength
=
= Gets the length of an explicit length chunk (not tiles)
= The file pointer is positioned so the compressed data can be read in next.
=
============================
*/

void CAL_GetGrChunkLength (short chunk)
{
	_lseek(grhandle,grstarts[chunk],SEEK_SET);
	_read(grhandle,&chunkexplen,sizeof(chunkexplen));
	chunkcomplen = grstarts[chunk+1]-grstarts[chunk]-4;
}


/*
==========================
=
= CA_FarRead
=
= Read from a file to a far pointer
=
==========================
*/

boolean CA_FarRead (int handle, byte far *dest, long length)
{
	if (length>0xffffl)
		Quit ("CA_FarRead doesn't support 64K reads yet!");

	return _read(handle, dest, length) == length;
}


/*
==========================
=
= CA_SegWrite
=
= Write from a file to a far pointer
=
==========================
*/

boolean CA_FarWrite (int handle, byte far *source, long length)
{
	if (length>0xffffl)
		Quit ("CA_FarWrite doesn't support 64K reads yet!");

	return _write(handle, source, length) == length;
}


/*
==========================
=
= CA_LoadFile
=
= Allocate space for and load a file
=
==========================
*/

boolean CA_LoadFile (char *filename, memptr *ptr)
{
	int handle;
	long size;

	if ((handle = _open(filename,O_RDONLY | O_BINARY, S_IREAD)) == -1)
		return false;

	size = _filelength (handle);
	MM_GetPtr (ptr,size);
	if (!CA_FarRead (handle,*ptr,size))
	{
		_close (handle);
		return false;
	}
	_close (handle);
	return true;
}

/*
======================
=
= CAL_HuffExpand
=
= Length is the length of the EXPANDED data
=
======================
*/

void CAL_HuffExpand(byte * __restrict source, byte * __restrict dest, long length, huffnode * __restrict hufftable)
{
	unsigned char * end = dest + length;
	short b;
	short m = 0x100;
	
	do
	{
		huffnode * __restrict node = hufftable + 254;

		for (;;)
		{
			int code;

			if (m & 0x100)
			{
				b = *source++;
				m = 1;
			}

			if ((b & m) == 0)
				code = node->bit0;
			else
				code = node->bit1;

			m <<= 1;

			if (code & 0x100)
				node = hufftable + (code & ~0x100);
			else
			{
				*dest++ = code;
				break;
			}
		}
	} while (dest != end);
}



/*
======================
=
= CA_RLEWcompress
=
======================
*/

long CA_RLEWCompress (unsigned short huge *source, long length, unsigned short huge *dest, unsigned short rlewtag)
{
  long complength;
  unsigned short value,count,i;
  unsigned short huge *start,huge *end;

  start = dest;

  end = source + (length+1)/2;

//
// compress it
//
  do
  {
    count = 1;
    value = *source++;
    while (*source == value && source<end)
    {
      count++;
      source++;
    }
    if (count>3 || value == rlewtag)
    {
    //
    // send a tag / count / value string
    //
      *dest++ = rlewtag;
      *dest++ = count;
      *dest++ = value;
    }
    else
    {
    //
    // send word without compressing
    //
      for (i=1;i<=count;i++)
	*dest++ = value;
	}

  } while (source<end);

  complength = 2*(dest-start);
  return complength;
}


/*
======================
=
= CA_RLEWexpand
= length is COMPRESSED length
=
======================
*/

void CA_RLEWexpand (unsigned short huge *source, unsigned short huge *dest, long length, unsigned short rlewtag)
{
  unsigned short value,count,i;
  unsigned short huge *end;
  unsigned short sourceseg,sourceoff,destseg,destoff,endseg,endoff;

  end = dest + (length)/2;

//
// expand it
//
  do
  {
	value = *source++;
	if (value != rlewtag)
	//
	// uncompressed
	//
	  *dest++=value;
	else
	{
	//
	// compressed string
	//
	  count = *source++;
	  value = *source++;
	  for (i=1;i<=count;i++)
	*dest++ = value;
	}
  } while (dest<end);
}



/*
=============================================================================

					 CACHE MANAGER ROUTINES

=============================================================================
*/


/*
======================
=
= CAL_SetupGrFile
=
======================
*/

void CAL_SetupGrFile (void)
{
	int handle;
	long headersize,length;
	memptr compseg;

#ifdef GRHEADERLINKED

	huffnode *grdict;

#if GRMODE == EGAGR
	grhuffman = (huffnode *)EGAdict;
	grstarts = (long _seg *)FP_SEG(EGAhead);
#endif

#else

//
// load ???dict.ext (huffman dictionary for graphics files)
//

//	if ((handle = _open(GREXT"DICT.",
	if ((handle = _open("KDREAMS.EGA",
		 O_RDONLY | O_BINARY, S_IREAD)) == -1)
		Quit ("Can't open KDREAMS.EGA!");

	_read(handle, &grhuffman, sizeof(grhuffman));
	_close(handle);
	CAL_OptimizeNodes (grhuffman);
//
// load the data offsets from ???head.ext
//
	MM_GetPtr (&(memptr)grstarts,(NUMCHUNKS+1)*4);

	if ((handle = _open(GREXT"HEAD."EXTENSION,
		 O_RDONLY | O_BINARY, S_IREAD)) == -1)
		Quit ("Can't open "GREXT"HEAD."EXTENSION"!");

	CA_FarRead(handle, (memptr)grstarts, (NUMCHUNKS+1)*4);

	_close(handle);


#endif

//
// Open the graphics file, leaving it open until the game is finished
//
//	grhandle = _open(GREXT"GRAPH."EXTENSION, O_RDONLY | O_BINARY); NOLAN
	grhandle = _open("KDREAMS.EGA", O_RDONLY | O_BINARY);
	if (grhandle == -1)
		Quit ("Cannot open KDREAMS.EGA!");


//
// load the pic and sprite headers into the arrays in the data segment
//
#if NUMPICS>0
	MM_GetPtr(&(memptr)pictable,NUMPICS*sizeof(pictabletype));
	CAL_GetGrChunkLength(STRUCTPIC);		// position file pointer
	MM_GetPtr(&compseg,chunkcomplen);
	CA_FarRead (grhandle,compseg,chunkcomplen);
	CAL_HuffExpand (compseg, (byte huge *)pictable,NUMPICS*sizeof(pictabletype),grhuffman);
	MM_FreePtr(&compseg);
#endif

#if NUMPICM>0
	MM_GetPtr(&(memptr)picmtable,NUMPICM*sizeof(pictabletype));
	CAL_GetGrChunkLength(STRUCTPICM);		// position file pointer
	MM_GetPtr(&compseg,chunkcomplen);
	CA_FarRead (grhandle,compseg,chunkcomplen);
	CAL_HuffExpand (compseg, (byte huge *)picmtable,NUMPICM*sizeof(pictabletype),grhuffman);
	MM_FreePtr(&compseg);
#endif

#if NUMSPRITES>0
	MM_GetPtr(&(memptr)spritetable,NUMSPRITES*sizeof(spritetabletype));
	CAL_GetGrChunkLength(STRUCTSPRITE);	// position file pointer
	MM_GetPtr(&compseg,chunkcomplen);
	CA_FarRead (grhandle,compseg,chunkcomplen);
	CAL_HuffExpand (compseg, (byte huge *)spritetable,NUMSPRITES*sizeof(spritetabletype),grhuffman);
	MM_FreePtr(&compseg);
#endif

}

//==========================================================================


/*
======================
=
= CAL_SetupMapFile
=
======================
*/

void CAL_SetupMapFile (void)
{
	int handle,i;
	long length;

//
// load maphead.ext (offsets and tileinfo for map file)
//
#ifndef MAPHEADERLINKED
//	if ((handle = _open("MAPHEAD."EXTENSION,
	if ((handle = _open("KDREAMS.MAP",
		 O_RDONLY | O_BINARY, S_IREAD)) == -1)
		Quit ("Can't open KDREAMS.MAP!");
	length = _filelength(handle);
	MM_GetPtr (&(memptr)tinf,length);
	CA_FarRead(handle, tinf, length);
	_close(handle);
#else

	maphuffman = (huffnode *)mapdict;
	tinf = (byte _seg *)FP_SEG(maphead);

#endif

//
// open the data file
//
#ifdef MAPHEADERLINKED
	if ((maphandle = _open("KDREAMS.MAP",
		 O_RDONLY | O_BINARY, S_IREAD)) == -1)
		Quit ("Can't open KDREAMS.MAP!");
#else
	if ((maphandle = _open("MAPTEMP."EXTENSION,
		 O_RDONLY | O_BINARY, S_IREAD)) == -1)
		Quit ("Can't open MAPTEMP."EXTENSION"!");
#endif
}

//==========================================================================


/*
======================
=
= CAL_SetupAudioFile
=
======================
*/

void CAL_SetupAudioFile (void)
{
	int handle,i;
	long length;

//
// load maphead.ext (offsets and tileinfo for map file)
//
#ifndef AUDIOHEADERLINKED
	if ((handle = _open("AUDIOHED."EXTENSION,
		 O_RDONLY | O_BINARY, S_IREAD)) == -1)
		Quit ("Can't open AUDIOHED."EXTENSION"!");
	length = filelength(handle);
	MM_GetPtr (&(memptr)audiostarts,length);
	CA_FarRead(handle, (byte far *)audiostarts, length);
	_close(handle);
#else
	audiohuffman = (huffnode *)audiodict;
	audiostarts = (long _seg *)FP_SEG(audiohead);
#endif

//
// open the data file
//
#ifndef AUDIOHEADERLINKED
	if ((audiohandle = _open("AUDIOT."EXTENSION,
		 O_RDONLY | O_BINARY, S_IREAD)) == -1)
		Quit ("Can't open AUDIOT."EXTENSION"!");
#else
//	if ((audiohandle = _open("AUDIO."EXTENSION,	NOLAN
	if ((audiohandle = _open("KDREAMS.AUD",
		 O_RDONLY | O_BINARY, S_IREAD)) == -1)
		Quit ("Can't open KDREAMS.AUD!");
#endif
}

//==========================================================================


/*
======================
=
= CA_Startup
=
= Open all files and load in headers
=
======================
*/

void CA_Startup (void)
{
#ifdef PROFILE
	unlink ("PROFILE.TXT");
	profilehandle = _open("PROFILE.TXT", O_CREAT | O_WRONLY | O_TEXT);
#endif

	CAL_SetupMapFile ();
	CAL_SetupGrFile ();
	CAL_SetupAudioFile ();

	mapon = -1;
	ca_levelbit = 1;
	ca_levelnum = 0;
}

//==========================================================================


/*
======================
=
= CA_Shutdown
=
= Closes all files
=
======================
*/

void CA_Shutdown (void)
{
#ifdef PROFILE
	if (profilehandle >= 0)
	{
		_close (profilehandle);
		profilehandle = -1;
	}
#endif

	if (audiohandle >= 0)
	{
		_close (audiohandle);
		audiohandle = -1;
	}
	if (maphandle >= 0)
	{
		_close (maphandle);
		maphandle = -1;
	}
	if (grhandle >= 0)
	{
		_close (grhandle);
		grhandle = -1;
	}
}

//===========================================================================

/*
======================
=
= CA_CacheAudioChunk
=
======================
*/

void CA_CacheAudioChunk (short chunk)
{
	long	pos,compressed,expanded;
	memptr	bigbufferseg;
	byte	far *source;

	if (audiosegs[chunk])
	{
		MM_SetPurge (&(memptr)audiosegs[chunk],0);
		return;							// allready in memory
	}

//
// load the chunk into a buffer, either the miscbuffer if it fits, or allocate
// a larger buffer
//
	pos = audiostarts[chunk];
	compressed = audiostarts[chunk+1]-pos;

	_lseek(audiohandle,pos,SEEK_SET);

#ifndef AUDIOHEADERLINKED

	MM_GetPtr (&(memptr)audiosegs[chunk],compressed);
	CA_FarRead(audiohandle,audiosegs[chunk],compressed);

#else

	if (compressed<=BUFFERSIZE)
	{
		CA_FarRead(audiohandle,bufferseg,compressed);
		source = bufferseg;
	}
	else
	{
		MM_GetPtr(&bigbufferseg,compressed);
		CA_FarRead(audiohandle,bigbufferseg,compressed);
		source = bigbufferseg;
	}

	expanded = *(long far *)source;
	source += 4;			// skip over length
	MM_GetPtr (&(memptr)audiosegs[chunk],expanded);
	CAL_HuffExpand (source,audiosegs[chunk],expanded,audiohuffman);

	if (compressed>BUFFERSIZE)
		MM_FreePtr(&bigbufferseg);
#endif
}

//===========================================================================

/*
======================
=
= CA_LoadAllSounds
=
= Purges all sounds, then loads all new ones (mode switch)
=
======================
*/

void CA_LoadAllSounds (void)
{
	unsigned short	start,i;

	switch (oldsoundmode)
	{
	case sdm_Off:
		goto cachein;
	case sdm_PC:
		start = STARTPCSOUNDS;
		break;
	case sdm_AdLib:
		start = STARTADLIBSOUNDS;
		break;
	case sdm_SoundBlaster:
	case sdm_SoundSource:
		start = STARTDIGISOUNDS;
		break;
	}

	for (i=0;i<NUMSOUNDS;i++,start++)
		if (audiosegs[start])
			MM_SetPurge (&(memptr)audiosegs[start],3);		// make purgable

cachein:

	switch (SoundMode)
	{
	case sdm_Off:
		return;
	case sdm_PC:
		start = STARTPCSOUNDS;
		break;
	case sdm_AdLib:
		start = STARTADLIBSOUNDS;
		break;
	case sdm_SoundBlaster:
	case sdm_SoundSource:
		start = STARTDIGISOUNDS;
		break;
	}

	for (i=0;i<NUMSOUNDS;i++,start++)
		CA_CacheAudioChunk (start);

	oldsoundmode = SoundMode;
}

//===========================================================================

#if GRMODE == EGAGR

/*
======================
=
= CAL_ShiftSprite
=
= Make a shifted (one byte wider) copy of a sprite into another area
=
======================
*/

void CAL_ShiftSprite (void * segment, unsigned short source, unsigned short dest, unsigned short width, unsigned short height, unsigned short pixshift)
{
	unsigned char * src = (unsigned char *)segment + source;
	unsigned char * dst = (unsigned char *)segment + dest;
	unsigned short y;

	const unsigned short * shifttable = shifttabletable[pixshift];

	// table shift the mask

	for (y = 0; y < height; ++y)
	{
		unsigned short x;

		*dst = 255; // // 0xff first byte

		for (x = 0; x < width; ++x)
		{
			// fetch unshifted source byte
			unsigned char value = ~(*src);

			// table shift into two bytes
			unsigned short shifted = ~shifttable[value];

			dst[0] &= (shifted >> 0) & 0xff; // and with first byte
			dst[1]  = (shifted >> 8) & 0xff; // replace next byte

			++src;
			++dst;
		}

		++dst;
	}

	// table shift the data

	for (y = height * 4; y != 0; --y) // four planes of data
	{
		unsigned short x;

		*dst = 0; // 0x00 first byte

		for (x = 0; x < width; ++x)
		{
			// fetch unshifted source byte
			unsigned char value = *src;

			// table shift into two bytes
			unsigned short shifted = shifttable[value];

			dst[0] |= (shifted >> 0) & 0xff; // or with first byte
			dst[1]  = (shifted >> 8) & 0xff; // replace next byte

			++src;
			++dst;
		}

		++dst; // the last shifted byte has 0s in it
	}
}

#endif

//===========================================================================

/*
======================
=
= CAL_CacheSprite
=
= Generate shifts and set up sprite structure for a given sprite
=
======================
*/

void CAL_CacheSprite (short chunk, char far *compressed)
{
	short int i;
	unsigned short shiftstarts[9]; // msnote : expanded from 5 to 9
	unsigned short smallplane,bigplane,expanded;
	spritetabletype far *spr;
	spritetype _seg *dest;

#if GRMODE == EGAGR

//
// calculate sizes
//
	spr = &spritetable[chunk-STARTSPRITES];
	smallplane = spr->width*spr->height;
	bigplane = (spr->width+1)*spr->height;

	shiftstarts[0] = MAXSHIFTS*6;	// start data after 3 unsigned tables
	shiftstarts[1] = shiftstarts[0] + smallplane*5;	// 5 planes in a sprite
	shiftstarts[2] = shiftstarts[1] + bigplane*5;
	shiftstarts[3] = shiftstarts[2] + bigplane*5;
	shiftstarts[4] = shiftstarts[3] + bigplane*5;
	shiftstarts[5] = shiftstarts[4] + bigplane*5;
	shiftstarts[6] = shiftstarts[5] + bigplane*5;
	shiftstarts[7] = shiftstarts[6] + bigplane*5;
	shiftstarts[8] = shiftstarts[7] + bigplane*5;	// nothing ever put here

#if SUPER_SMOOTH_SCROLLING
	spr->shifts = 8;
#endif

	expanded = shiftstarts[spr->shifts];
	MM_GetPtr (&grsegs[chunk],expanded);
	dest = (spritetype _seg *)grsegs[chunk];

//
// expand the unshifted shape
//
	CAL_HuffExpand (compressed, &dest->data[0],smallplane*5,grhuffman);

//
// make the shifts!
//
	switch (spr->shifts)
	{
	case	1:
		for (i=0;i<4;i++)
		{
			dest->sourceoffset[i] = shiftstarts[0];
			dest->planesize[i] = smallplane;
			dest->width[i] = spr->width;
		}
		break;

	case	2:
		for (i=0;i<2;i++)
		{
			dest->sourceoffset[i] = shiftstarts[0];
			dest->planesize[i] = smallplane;
			dest->width[i] = spr->width;
		}
		for (i=2;i<4;i++)
		{
			dest->sourceoffset[i] = shiftstarts[1];
			dest->planesize[i] = bigplane;
			dest->width[i] = spr->width+1;
		}
		CAL_ShiftSprite (grsegs[chunk],dest->sourceoffset[0],
			dest->sourceoffset[2],spr->width,spr->height,4);
		break;

	case	4:
		dest->sourceoffset[0] = shiftstarts[0];
		dest->planesize[0] = smallplane;
		dest->width[0] = spr->width;

		dest->sourceoffset[1] = shiftstarts[1];
		dest->planesize[1] = bigplane;
		dest->width[1] = spr->width+1;
		CAL_ShiftSprite (grsegs[chunk],dest->sourceoffset[0],
			dest->sourceoffset[1],spr->width,spr->height,2);

		dest->sourceoffset[2] = shiftstarts[2];
		dest->planesize[2] = bigplane;
		dest->width[2] = spr->width+1;
		CAL_ShiftSprite (grsegs[chunk],dest->sourceoffset[0],
			dest->sourceoffset[2],spr->width,spr->height,4);

		dest->sourceoffset[3] = shiftstarts[3];
		dest->planesize[3] = bigplane;
		dest->width[3] = spr->width+1;
		CAL_ShiftSprite (grsegs[chunk],dest->sourceoffset[0],
			dest->sourceoffset[3],spr->width,spr->height,6);

		break;

#if SUPER_SMOOTH_SCROLLING
	case	8:
		dest->sourceoffset[0] = shiftstarts[0];
		dest->planesize[0] = smallplane;
		dest->width[0] = spr->width;

		for (i = 1; i < 8; ++i)
		{
			dest->sourceoffset[i] = shiftstarts[i];
			dest->planesize[i] = bigplane;
			dest->width[i] = spr->width+1;
			CAL_ShiftSprite (grsegs[chunk],dest->sourceoffset[0],
				dest->sourceoffset[i],spr->width,spr->height,i);
		}

		break;
#endif

	default:
		Quit ("CAL_CacheSprite: Bad shifts number!");
	}

#endif
}

//===========================================================================


/*
======================
=
= CAL_ExpandGrChunk
=
= Does whatever is needed with a pointer to a compressed chunk
=
======================
*/

void CAL_ExpandGrChunk (short chunk, byte far *source)
{
	long	pos,expanded;
	short	next;
	spritetabletype	*spr;


	if (chunk>=STARTTILE8)
	{
	//
	// expanded sizes of tile8/16/32 are implicit
	//

#if GRMODE == EGAGR
#define BLOCK		32
#define MASKBLOCK	40
#endif

		if (chunk<STARTTILE8M)			// tile 8s are all in one chunk!
			expanded = BLOCK*NUMTILE8;
		else if (chunk<STARTTILE16)
			expanded = MASKBLOCK*NUMTILE8M;
		else if (chunk<STARTTILE16M)	// all other tiles are one/chunk
			expanded = BLOCK*4;
		else if (chunk<STARTTILE32)
			expanded = MASKBLOCK*4;
		else if (chunk<STARTTILE32M)
			expanded = BLOCK*16;
		else
			expanded = MASKBLOCK*16;
	}
	else
	{
	//
	// everything else has an explicit size longword
	//
		expanded = *(long far *)source;
		source += 4;			// skip over length
	}

//
// allocate final space, decompress it, and free bigbuffer
// Sprites need to have shifts made and various other junk
//
	if (chunk>=STARTSPRITES && chunk< STARTTILE8)
		CAL_CacheSprite(chunk,source);
	else
	{
		MM_GetPtr (&grsegs[chunk],expanded);
		CAL_HuffExpand (source,grsegs[chunk],expanded,grhuffman);
	}
}


/*
======================
=
= CAL_ReadGrChunk
=
= Gets a chunk off disk, optimizing reads to general buffer
=
======================
*/

void CAL_ReadGrChunk (short chunk)
{
	long	pos,compressed;
	memptr	bigbufferseg;
	byte	far *source;
	short	next;
	spritetabletype	*spr;

//
// load the chunk into a buffer, either the miscbuffer if it fits, or allocate
// a larger buffer
//
	pos = grstarts[chunk];
	if (pos<0)							// $FFFFFFFF start is a sparse tile
	  return;

	next = chunk +1;
	while (grstarts[next] == -1)		// skip past any sparse tiles
		next++;

	compressed = grstarts[next]-pos;

	_lseek(grhandle,pos,SEEK_SET);

	if (compressed<=BUFFERSIZE)
	{
		CA_FarRead(grhandle,bufferseg,compressed);
		source = bufferseg;
	}
	else
	{
		MM_GetPtr(&bigbufferseg,compressed);
		CA_FarRead(grhandle,bigbufferseg,compressed);
		source = bigbufferseg;
	}

	CAL_ExpandGrChunk (chunk,source);

	if (compressed>BUFFERSIZE)
		MM_FreePtr(&bigbufferseg);
}


/*
======================
=
= CA_CacheGrChunk
=
= Makes sure a given chunk is in memory, loadiing it if needed
=
======================
*/

void CA_CacheGrChunk (short chunk)
{
	long	pos,compressed;
	memptr	bigbufferseg;
	byte	far *source;
	short	next;

	grneeded[chunk] |= ca_levelbit;		// make sure it doesn't get removed
	if (grsegs[chunk])
	  return;							// allready in memory

//
// load the chunk into a buffer, either the miscbuffer if it fits, or allocate
// a larger buffer
//
	pos = grstarts[chunk];
	if (pos<0)							// $FFFFFFFF start is a sparse tile
	  return;

	next = chunk +1;
	while (grstarts[next] == -1)		// skip past any sparse tiles
		next++;

	compressed = grstarts[next]-pos;

	_lseek(grhandle,pos,SEEK_SET);

	if (compressed<=BUFFERSIZE)
	{
		CA_FarRead(grhandle,bufferseg,compressed);
		source = bufferseg;
	}
	else
	{
		MM_GetPtr(&bigbufferseg,compressed);
		CA_FarRead(grhandle,bigbufferseg,compressed);
		source = bigbufferseg;
	}

	CAL_ExpandGrChunk (chunk,source);

	if (compressed>BUFFERSIZE)
		MM_FreePtr(&bigbufferseg);
}



//==========================================================================

/*
======================
=
= CA_CacheMap
=
======================
*/

void CA_CacheMap (short mapnum)
{
	long			pos,compressed,expanded;
	short int		plane;
	memptr			*dest,bigbufferseg,buffer2seg;
	unsigned short	size;
	unsigned short	far	*source;


//
// free up memory from last map
//
	if (mapon>-1 && mapheaderseg[mapon])
		MM_SetPurge (&(memptr)mapheaderseg[mapon],3);
	for (plane=0;plane<3;plane++)
		if (mapsegs[plane])
			MM_FreePtr (&(memptr)mapsegs[plane]);

	mapon = mapnum;


//
// load map header
// The header will be cached if it is still around
//
	if (!mapheaderseg[mapnum])
	{
		pos = ((mapfiletype	_seg *)tinf)->headeroffsets[mapnum];
		if (pos<0)						// $FFFFFFFF start is a sparse map
		  Quit ("CA_CacheMap: Tried to load a non existant map!");

		MM_GetPtr(&(memptr)mapheaderseg[mapnum],sizeof(maptype));
		_lseek(maphandle,pos,SEEK_SET);

#ifdef MAPHEADERLINKED
		//
		// load in, then unhuffman to the destination
		//
		CA_FarRead (maphandle,bufferseg,((mapfiletype	_seg *)tinf)->headersize[mapnum]);
		CAL_HuffExpand ((byte huge *)bufferseg,
			(byte huge *)mapheaderseg[mapnum],sizeof(maptype),maphuffman);
#else
		CA_FarRead (maphandle,(memptr)mapheaderseg[mapnum],sizeof(maptype));
#endif
	}
	else
		MM_SetPurge (&(memptr)mapheaderseg[mapnum],0);

//
// load the planes in
// If a plane's pointer still exists it will be overwritten (levels are
// allways reloaded, never cached)
//

	size = mapheaderseg[mapnum]->width * mapheaderseg[mapnum]->height * 2;

	for (plane = 0; plane<3; plane++)
	{
		dest = &(memptr)mapsegs[plane];
		MM_GetPtr(dest,size);

		pos = mapheaderseg[mapnum]->planestart[plane];
		compressed = mapheaderseg[mapnum]->planelength[plane];
		_lseek(maphandle,pos,SEEK_SET);
		if (compressed<=BUFFERSIZE)
			source = bufferseg;
		else
		{
			MM_GetPtr(&bigbufferseg,compressed);
			source = bigbufferseg;
		}

		CA_FarRead(maphandle,(byte far *)source,compressed);
#ifdef MAPHEADERLINKED
		//
		// unhuffman, then unRLEW
		// The huffman'd chunk has a two byte expanded length first
		// The resulting RLEW chunk also does, even though it's not really
		// needed
		//
		expanded = *source;
		source++;
		MM_GetPtr (&buffer2seg,expanded);
		CAL_HuffExpand ((byte huge *)source, buffer2seg,expanded,maphuffman);
		CA_RLEWexpand (((unsigned short far *)buffer2seg)+1,*dest,size, ((mapfiletype _seg *)tinf)->RLEWtag);
		MM_FreePtr (&buffer2seg);

#else
		//
		// unRLEW, skipping expanded length
		//
		CA_RLEWexpand (source+1, *dest,size,
		((mapfiletype _seg *)tinf)->RLEWtag);
#endif

		if (compressed>BUFFERSIZE)
			MM_FreePtr(&bigbufferseg);
	}
}

//===========================================================================

/*
======================
=
= CA_UpLevel
=
= Goes up a bit level in the needed lists and clears it out.
= Everything is made purgable
=
======================
*/

void CA_UpLevel (void)
{
	if (ca_levelnum==7)
		Quit ("CA_UpLevel: Up past level 7!");

	ca_levelbit<<=1;
	ca_levelnum++;
}

//===========================================================================

/*
======================
=
= CA_DownLevel
=
= Goes down a bit level in the needed lists and recaches
= everything from the lower level
=
======================
*/

void CA_DownLevel (void)
{
	if (!ca_levelnum)
		Quit ("CA_DownLevel: Down past level 0!");
	ca_levelbit>>=1;
	ca_levelnum--;
	CA_CacheMarks(titleptr[ca_levelnum], 1);
}

//===========================================================================

/*
======================
=
= CA_ClearMarks
=
= Clears out all the marks at the current level
=
======================
*/

void CA_ClearMarks (void)
{
	short i;

	for (i=0;i<NUMCHUNKS;i++)
		grneeded[i]&=~ca_levelbit;
}


//===========================================================================

/*
======================
=
= CA_ClearAllMarks
=
= Clears out all the marks on all the levels
=
======================
*/

void CA_ClearAllMarks (void)
{
	memset (grneeded,0,sizeof(grneeded));
	ca_levelbit = 1;
	ca_levelnum = 0;
}


//===========================================================================


/*
======================
=
= CA_CacheMarks
=
======================
*/

#define NUMBARS	(17l*8)
#define BARSTEP	8
#define MAXEMPTYREAD	1024

void CA_CacheMarks (char *title, boolean cachedownlevel)
{
	boolean dialog;
	short 	i,next,homex,homey,x,y,thx,thy,numcache,lastx,xl,xh;
	long	barx,barstep;
	long	pos,endpos,nextpos,nextendpos,compressed;
	long	bufferstart,bufferend;	// file position of general buffer
	byte	far *source;
	memptr	bigbufferseg;

	//
	// save title so cache down level can redraw it
	//
	titleptr[ca_levelnum] = title;

	dialog = (title!=NULL);

	if (cachedownlevel)
		dialog = false;

	if (dialog)
	{
	//
	// draw dialog window (masked tiles 12 - 20 are window borders)
	//
		US_CenterWindow (20,8);
		homex = PrintX;
		homey = PrintY;

		US_CPrint ("Loading");
		fontcolor = F_SECONDCOLOR;
		US_CPrint (title);
		fontcolor = F_BLACK;
		VW_UpdateScreen();
#ifdef PROFILE
		_write(profilehandle,title,strlen(title));
		_write(profilehandle,"\n",1);
#endif

	}

	numcache = 0;
//
// go through and make everything not needed purgable
//
	for (i=0;i<NUMCHUNKS;i++)
		if (grneeded[i]&ca_levelbit)
		{
			if (grsegs[i])					// its allready in memory, make
				MM_SetPurge(&grsegs[i],0);	// sure it stays there!
			else
				numcache++;
		}
		else
		{
			if (grsegs[i])					// not needed, so make it purgeable
				MM_SetPurge(&grsegs[i],3);
		}

	if (!numcache)			// nothing to cache!
		return;

	if (dialog)
	{
	//
	// draw thermometer bar
	//
		thx = homex + 8;
		thy = homey + 32;
		VWB_DrawTile8(thx,thy,11);
		VWB_DrawTile8(thx,thy+8,14);
		VWB_DrawTile8(thx,thy+16,17);
		VWB_DrawTile8(thx+17*8,thy,13);
		VWB_DrawTile8(thx+17*8,thy+8,16);
		VWB_DrawTile8(thx+17*8,thy+16,19);
		for (x=thx+8;x<thx+17*8;x+=8)
		{
			VWB_DrawTile8(x,thy,12);
			VWB_DrawTile8(x,thy+8,15);
			VWB_DrawTile8(x,thy+16,18);
		}

		thx += 4;		// first line location
		thy += 5;
		barx = (long)thx<<16;
		lastx = thx;
		VW_UpdateScreen();
	}

//
// go through and load in anything still needed
//
	barstep = (NUMBARS<<16)/numcache;
	bufferstart = bufferend = 0;		// nothing good in buffer now

	for (i=0;i<NUMCHUNKS;i++)
		if ( (grneeded[i]&ca_levelbit) && !grsegs[i])
		{
//
// update thermometer
//
			if (dialog)
			{
				barx+=barstep;
				xh = barx>>16;
				if (xh - lastx > BARSTEP)
				{
					for (x=lastx;x<=xh;x++)
#if GRMODE == EGAGR
						VWB_Vlin (thy,thy+13,x,14);
#endif
					lastx = xh;
					VW_UpdateScreen();

					SDL_Delay(10); // msfixme
				}

			}
			pos = grstarts[i];
			if (pos<0)
				continue;

			next = i +1;
			while (grstarts[next] == -1)		// skip past any sparse tiles
				next++;

			compressed = grstarts[next]-pos;
			endpos = pos+compressed;

			if (compressed<=BUFFERSIZE)
			{
				if (bufferstart<=pos
				&& bufferend>= endpos)
				{
				// data is allready in buffer
					source = ((byte _seg *)bufferseg)+(pos-bufferstart);
				}
				else
				{
				// load buffer with a new block from disk
				// try to get as many of the needed blocks in as possible
					while ( next < NUMCHUNKS )
					{
						while (next < NUMCHUNKS &&
						!(grneeded[next]&ca_levelbit && !grsegs[next]))
							next++;
						if (next == NUMCHUNKS)
							continue;

						nextpos = grstarts[next];
						while (grstarts[++next] == -1)	// skip past any sparse tiles
							;
						nextendpos = grstarts[next];
						if (nextpos - endpos <= MAXEMPTYREAD
						&& nextendpos-pos <= BUFFERSIZE)
							endpos = nextendpos;
						else
							next = NUMCHUNKS;			// read pos to posend
					}

					_lseek(grhandle,pos,SEEK_SET);
					CA_FarRead(grhandle,bufferseg,endpos-pos);
					bufferstart = pos;
					bufferend = endpos;
					source = bufferseg;
				}
			}
			else
			{
			// big chunk, allocate temporary buffer
				MM_GetPtr(&bigbufferseg,compressed);
				_lseek(grhandle,pos,SEEK_SET);
				CA_FarRead(grhandle,bigbufferseg,compressed);
				source = bigbufferseg;
			}

			CAL_ExpandGrChunk (i,source);

			if (compressed>BUFFERSIZE)
				MM_FreePtr(&bigbufferseg);

		}

//
// finish up any thermometer remnants
//
		if (dialog)
		{
			xh = thx + NUMBARS;
			for (x=lastx;x<=xh;x++)
#if GRMODE == EGAGR
				VWB_Vlin (thy,thy+13,x,14);
#endif
			VW_UpdateScreen();
		}
}

