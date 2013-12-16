#pragma once

#include "klodder_forward.h"

void BitmapToMacImage(const Bitmap* bitmap, MacImage* image);
void MacImageToBitmap(const MacImage* image, Bitmap* bitmap);
void BitmapToMacImage(Bitmap* src, MacImage* dst, int x, int y, int sx, int sy);
void MacImageToBitmap(MacImage* src, Bitmap* dst, int x, int y);
