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

// ID_CA.H

#ifndef __TYPES__
#include "ID_TYPES.H"
#endif

#ifndef __ID_MM__
#include "ID_MM.H"
#endif

#ifndef __ID_GLOB__
#include "ID_GLOB.H"
#endif

#define __ID_CA__

//===========================================================================

#define MAPHEADERLINKED
#define GRHEADERLINKED
#define AUDIOHEADERLINKED

#define NUMMAPS	30

//
// tile info defines, as bytes after tinf the table starts
//


#define	SPEED	502
#define ANIM	(SPEED+NUMTILE16)

#define NORTHWALL	(ANIM+NUMTILE16)
#define EASTWALL	(NORTHWALL+NUMTILE16M)
#define SOUTHWALL   (EASTWALL+NUMTILE16M)
#define WESTWALL    (SOUTHWALL+NUMTILE16M)
#define MANIM       (WESTWALL+NUMTILE16M)
#define INTILE      (MANIM+NUMTILE16M)
#define MSPEED      (INTILE+NUMTILE16M)

//===========================================================================

#pragma pack(push)
#pragma pack(2)

typedef	struct
{
	long			planestart[3];
	unsigned short	planelength[3];
	unsigned short	width,height;
	char			name[16];
} maptype;

#pragma pack(pop)

//===========================================================================

extern	byte 		_seg	*tinf;
extern	short		mapon;

extern	unsigned short	_seg	*mapsegs[3];
extern	maptype			_seg	*mapheaderseg[NUMMAPS];
extern	byte			_seg	*audiosegs[NUMSNDCHUNKS];
extern	void			_seg	*grsegs[NUMCHUNKS];

extern	byte		grneeded[NUMCHUNKS];
extern	byte		ca_levelbit,ca_levelnum;

extern	char		*titleptr[8];

extern	int			profilehandle;

//===========================================================================

// just for the score box reshifting

void CAL_ShiftSprite (void * segment, unsigned short source, unsigned short dest, unsigned short width, unsigned short height, unsigned short pixshift);

//===========================================================================

boolean CA_FarRead (int handle, byte far *dest, long length);
boolean CA_FarWrite (int handle, byte far *source, long length);
boolean CA_LoadFile (char *filename, memptr *ptr);

long CA_RLEWCompress (unsigned short huge *source, long length, unsigned short huge *dest, unsigned short rlewtag);

void CA_RLEWexpand (unsigned short huge *source, unsigned short huge *dest, long length, unsigned short rlewtag);

void CA_Startup (void);
void CA_Shutdown (void);

void CA_CacheAudioChunk (short chunk);
void CA_LoadAllSounds (void);

void CA_UpLevel (void);
void CA_DownLevel (void);

void CA_ClearMarks (void);
void CA_ClearAllMarks (void);

#define CA_MarkGrChunk(chunk)	grneeded[chunk]|=ca_levelbit

void CA_CacheGrChunk (short chunk);
void CA_CacheMap (short mapnum);

void CA_CacheMarks (char *title, boolean cachedownlevel);

