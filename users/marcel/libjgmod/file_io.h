#pragma once

#include <stdio.h>

// -- located in file_io.c ---------------------------------------------------
FILE *jgmod_fopen (const char *filename, const char *mode);
int jgmod_fclose (FILE *f);
void jgmod_fseek (FILE **f, const char *filename, int offset);
void jgmod_skip (FILE *f, int skip);
int jgmod_fread (void *buf, int size, FILE *f);
int jgmod_getc (FILE *f);
int jgmod_mgetw (FILE *f);
long jgmod_mgetl (FILE *f);
int jgmod_igetw (FILE *f);
long jgmod_igetl (FILE *f);
int jgmod_putc (int c, FILE *f);
int jgmod_iputw (int w, FILE *f);
long jgmod_iputl (long w, FILE *f);
int jgmod_fwrite (const void *buf, int size, FILE *f);
