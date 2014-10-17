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

#include "KD_DEF.H"
#include "syscode.h"

#define BIO_BUFFER_LEN	(512)



////////////////////////////////////////////////////////////////////////////
//
// FreeShape()
//
void FreeShape(struct Shape *shape)
{
	if (shape->Data)
		MM_FreePtr(&shape->Data);
}

////////////////////////////////////////////////////////////////////////////
//
// UnpackEGAShapeToScreen()
//
short UnpackEGAShapeToScreen(struct Shape *SHP, short startx, short starty)
{
	short currenty;
	signed char n, Rep;
	unsigned char far *Src, far *Dst[8], loop, Plane;
	unsigned short BPR, Height;
	short NotWordAligned;

	NotWordAligned = SHP->BPR & 1;
	startx>>=3;
	Src = SHP->Data;
	currenty = starty;
	Plane = 0;
	Height = SHP->bmHdr.h;
	while (Height--)
	{
		for (loop=0; loop<SHP->bmHdr.d; loop++)
		{
			Dst[loop] = &g0xA000[loop][displayofs];
			Dst[loop] += ylookup[currenty];
			Dst[loop] += startx;
		}

		for (Plane=0; Plane<SHP->bmHdr.d; Plane++)
		{
			BPR = ((SHP->BPR+1) >> 1) << 1;               // IGNORE WORD ALIGN
			while (BPR)
			{
				if (SHP->bmHdr.comp)
					n = *Src++;
				else
					n = BPR-1;

				if (n < 0)
				{
					if (n != -128)
					{
						n = (-n)+1;
						BPR -= n;
						Rep = *Src++;
						if ((!BPR) && (NotWordAligned))   // IGNORE WORD ALIGN
							n--;

						while (n--)
							*Dst[Plane]++ = Rep;
					}
					else
						BPR--;
				}
				else
				{
					n++;
					BPR -= n;
					if ((!BPR) && (NotWordAligned))     // IGNORE WORD ALIGN
						n--;

					while (n--)
						*Dst[Plane]++ = *Src++;

					if ((!BPR) && (NotWordAligned))     // IGNORE WORD ALIGN
						Src++;
				}
			}
		}
		currenty++;
	}

	return(0);
}

////////////////////////////////////////////////////////////////////////////
//
// Verify()
//
int32_t Verify(char *filename)
{
	int handle;
	int32_t size;

	if ((handle=_open(filename,O_BINARY))==-1)
		return (0);
	size=_filelength(handle);
	_close(handle);
	return(size);
}

//--------------------------------------------------------------------------
// InitBufferedIO()
//--------------------------------------------------------------------------
memptr InitBufferedIO(int handle, BufferedIO *bio)
{
	bio->handle = handle;
	bio->offset = BIO_BUFFER_LEN;
	bio->status = 0;
	MM_GetPtr(&bio->buffer,BIO_BUFFER_LEN);

	return(bio->buffer);
}

//--------------------------------------------------------------------------
// FreeBufferedIO()
//--------------------------------------------------------------------------
void FreeBufferedIO(BufferedIO *bio)
{
	if (bio->buffer)
		MM_FreePtr(&bio->buffer);
}

//--------------------------------------------------------------------------
// bio_readch()
//--------------------------------------------------------------------------
byte bio_readch(BufferedIO *bio)
{
	byte far *buffer;

	if (bio->offset == BIO_BUFFER_LEN)
	{
		bio->offset = 0;
		bio_fillbuffer(bio);
	}

	buffer = MK_FP(bio->buffer,bio->offset++);

	return(*buffer);
}

//--------------------------------------------------------------------------
// bio_fillbuffer()
//
// BUGS (Not really bugs... More like RULES!)
//
//    1) This code assumes BIO_BUFFER_LEN is no smaller than
//       NEAR_BUFFER_LEN!!
//
//    2) BufferedIO.status should be altered by this code to report
//       read errors, end of file, etc... If you know how big the file
//       is you're reading, determining EOF should be no problem.
//
//--------------------------------------------------------------------------
void bio_fillbuffer(BufferedIO *bio)
{
	#define NEAR_BUFFER_LEN	(64)
	byte near_buffer[NEAR_BUFFER_LEN];
	short bio_length,bytes_read,bytes_requested;

	bytes_read = 0;
	bio_length = BIO_BUFFER_LEN;
	while (bio_length)
	{
		if (bio_length > NEAR_BUFFER_LEN-1)
			bytes_requested = NEAR_BUFFER_LEN;
		else
			bytes_requested = bio_length;

		_read(bio->handle,near_buffer,bytes_requested);
		memcpy(MK_FP(bio->buffer,bytes_read),near_buffer,bytes_requested);

		bio_length -= bytes_requested;
		bytes_read += bytes_requested;
	}
}

///////////////////////////////////////////////////////////////////////////
//
// SwapLong()
//
void SwapLong(int32_t far *Var)
{
	char temp;
	char * bytes = (char*)Var;
	temp = bytes[0];
	bytes[0] = bytes[3];
	bytes[3] = temp;
	temp = bytes[2];
	bytes[2] = bytes[1];
	bytes[1] = temp;
}

///////////////////////////////////////////////////////////////////////////
//
// SwapWord()
//
void SwapWord(uint16_t far *Var)
{
	char temp;
	char * bytes = (char*)Var;
	temp = bytes[0];
	bytes[0] = bytes[1];
	bytes[1] = temp;
}

////////////////////////////////////////////////////////////////////////////
//
// MoveGfxDst()
//
void MoveGfxDst(short x, short y)
{
	unsigned address;

	address = (y*linewidth)+(x/8);
	bufferofs = displayofs = address;
}
