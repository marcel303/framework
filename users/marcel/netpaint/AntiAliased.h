#pragma once

struct BITMAP;

// 2x2 anti-aliased graphics functions - Marcel Smit, 2001
// these use 16.16 fixed point coordinates

void aa_putpixel(BITMAP* dst, int x, int y, int c);
void aa_putpixel8(BITMAP* dst, int x, int y, int c);
void aa_putpixel15(BITMAP* dst, int x, int y, int c);
void aa_putpixel16(BITMAP* dst, int x, int y, int c);
void aa_putpixel24(BITMAP* dst, int x, int y, int c);
void aa_putpixel32(BITMAP* dst, int x, int y, int c);

void aa_line(BITMAP* dst, int x1, int y1, int x2, int y2, int c);
void aa_line8(BITMAP* dst, int x1, int y1, int x2, int y2, int c);
void aa_line15(BITMAP* dst, int x1, int y1, int x2, int y2, int c);
void aa_line16(BITMAP* dst, int x1, int y1, int x2, int y2, int c);
void aa_line24(BITMAP* dst, int x1, int y1, int x2, int y2, int c);
void aa_line32(BITMAP* dst, int x1, int y1, int x2, int y2, int c);
