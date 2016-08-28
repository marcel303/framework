#pragma once

#include "libklodder_forward.h"

void BitmapToMacImage(const Bitmap * bitmap, MacImage * image);
void MacImageToBitmap(const MacImage * image, Bitmap * bitmap);
void BitmapToMacImage(const Bitmap * src, MacImage * dst, const int x, const int y, const int sx, const int sy);
void MacImageToBitmap(const MacImage * src, Bitmap * dst, const int x, const int y);
