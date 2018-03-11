#ifndef FILE_IO_H
#define FILE_IO_H

#include "port.h"

// -- located in file_io.c ---------------------------------------------------
JGMOD_FILE *jgmod_fopen (char *filename, char *mode);
int jgmod_fclose (JGMOD_FILE *f);
void jgmod_fseek (JGMOD_FILE **f, char *filename, int offset);
void jgmod_skip (JGMOD_FILE *f, int skip);
int jgmod_fread (char *buf, int size, JGMOD_FILE *f);
int jgmod_getc (JGMOD_FILE *f);
int jgmod_mgetw (JGMOD_FILE *f);
long jgmod_mgetl (JGMOD_FILE *f);
int jgmod_igetw (JGMOD_FILE *f);
long jgmod_igetl (JGMOD_FILE *f);
int jgmod_putc (int c, JGMOD_FILE *f);
int jgmod_iputw (int w, JGMOD_FILE *f);
long jgmod_iputl (long w, JGMOD_FILE *f);
int jgmod_fwrite (void *buf, int size, JGMOD_FILE *f);

#endif
