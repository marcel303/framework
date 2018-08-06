/*    
 *
 *                _______  _______  __________  _______  _____ 
 *               /____  / / _____/ /         / / ___  / / ___ \
 *               __  / / / / ____ / //   // / / /  / / / /  / /
 *             /  /_/ / / /__/ / / / /_/ / / / /__/ / / /__/ /
 *            /______/ /______/ /_/     /_/ /______/ /______/
 *
 *
 *
 *
 *  File IO routines. For portability reasons. */

#include "file_io.h"
#include "jgmod.h"

#include "framework-allegro2.h"

#include <stdio.h>

JGMOD_FILE *jgmod_fopen (const char *filename, const char *mode)
{
#ifdef JGMOD_PACKFILE
    return pack_fopen (filename, mode);
#else
    return fopen(filename, mode);
#endif
}


int jgmod_fclose (JGMOD_FILE *f)
{
#ifdef JGMOD_PACKFILE
    return pack_fclose (f);
#else
    return fclose (f);
#endif
}


void jgmod_fseek (JGMOD_FILE **f, const char *filename, int offset)
{
#ifdef JGMOD_PACKFILE
    pack_fclose (*f);
    *f = pack_fopen (filename, "r");
    pack_fseek (*f, offset);
#else
    fseek (*f, offset, SEEK_SET);
#endif
}


void jgmod_skip (JGMOD_FILE *f, int skip)
{
#ifdef JGMOD_PACKFILE
    pack_fseek (f, skip);
#else
    fseek (f, skip, SEEK_CUR);
#endif
}


int jgmod_fread (void *buf, int size, JGMOD_FILE *f)
{
#ifdef JGMOD_PACKFILE
    return pack_fread (buf, size, f);
#else
    return fread (buf, 1, size, f);
#endif
}


int jgmod_getc (JGMOD_FILE *f)
{
#ifdef JGMOD_PACKFILE
    return pack_getc (f);
#else
    return getc (f);
#endif
}


int jgmod_mgetw (JGMOD_FILE *f)
{
    int b1, b2;

    if ( (b1=jgmod_getc(f)) != EOF)
        if ( (b2=jgmod_getc(f)) != EOF)
            return ( (b1 << 8) + b2 );

    return EOF;
}


long jgmod_mgetl (JGMOD_FILE *f)
{
    long b1, b2, b3, b4;

    if ( (b1=jgmod_getc(f)) != EOF)
        if ( (b2=jgmod_getc(f)) != EOF)
            if ( (b3=jgmod_getc(f)) != EOF)
                if ( (b4=jgmod_getc(f)) != EOF)
                    return ( (b1 << 24) + (b2 << 16) + (b3 << 8) + b4 );

    return EOF;
}


int jgmod_igetw (JGMOD_FILE *f)
{
    int b1, b2;

    if ( (b1=jgmod_getc(f)) != EOF)
        if ( (b2=jgmod_getc(f)) != EOF)
            return ( (b2 << 8) + b1 );

    return EOF;
}


long jgmod_igetl (JGMOD_FILE *f)
{
    long b1, b2, b3, b4;

    if ( (b1=jgmod_getc(f)) != EOF)
        if ( (b2=jgmod_getc(f)) != EOF)
            if ( (b3=jgmod_getc(f)) != EOF)
                if ( (b4=jgmod_getc(f)) != EOF)
                    return ( (b4 << 24) + (b3 << 16) + (b2 << 8) + b1 );

    return EOF;
}


int jgmod_putc (int c, JGMOD_FILE *f)
{
#ifdef JGMOD_PACKFILE
    return pack_putc (c, f);
#else
    return putc (c, f);
#endif
}


int jgmod_iputw (int w, JGMOD_FILE *f)
{
    int b1, b2;

    b1 = (w & 0xFF00) >> 8;
    b2 = (w & 0xFF);

   if (jgmod_putc(b2,f)==b2)
      if (jgmod_putc(b1,f)==b1)
         return w;

    return EOF;
}


long jgmod_iputl (long l, JGMOD_FILE *f)
{
    int b1, b2, b3, b4;

    b1 = (long)((l & 0xFF000000L) >> 24);
    b2 = (long)((l & 0x00FF0000L) >> 16);
    b3 = (long)((l & 0x0000FF00L) >> 8);
    b4 = (long)l & 0x00FF;

    if (jgmod_putc(b4,f)==b4)
        if (jgmod_putc(b3,f)==b3)
            if (jgmod_putc(b2,f)==b2)
                if (jgmod_putc(b1,f)==b1)
                    return l;

   return EOF;
}


int jgmod_fwrite (const void *buf, int size, JGMOD_FILE *f)
{
#ifdef JGMOD_PACKFILE
    return pack_fwrite (buf, size, f);
#else
    return fwrite (buf, sizeof(char), size, f);
#endif
}
