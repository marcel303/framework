#include <allegro.h>
#include <assert.h>
#include <stdio.h>
#include "Image.h"

static char * format_filename(const char* filename)
{
	static char temp[256];
	sprintf(temp, "data/%s", filename);
	return temp;
}

#if 1

BITMAP* load_bitmap_il(const char* filename)
{
	filename = format_filename(filename);

	BITMAP* bmp = load_bitmap(filename, NULL);
	if (bmp)
		return bmp;
	else
		return create_bitmap(SCREEN_W, SCREEN_H);
}

#else

#include <IL/IL.h>

BITMAP* load_bitmap_il(const char* filename)
{
	filename = format_filename(filename);

	ilOriginFunc(IL_ORIGIN_UPPER_LEFT);
	ilEnable(IL_ORIGIN_SET);

	ilLoadImage((ILstring)filename);
	assert(ilGetError() == IL_NO_ERROR);

	ilConvertImage(IL_RGBA, IL_BYTE);
	assert(ilGetError() == IL_NO_ERROR);

	const int w = ilGetInteger(IL_IMAGE_WIDTH);
	const int h = ilGetInteger(IL_IMAGE_HEIGHT);

	const void* data = ilGetData();
	assert(ilGetError() == IL_NO_ERROR);

	BITMAP* bmp = create_bitmap_ex(32, w, h);

	for (int y = 0; y < h; ++y)
	{
		const unsigned char* line = &((const unsigned char*)data)[y * w * 4];
		//const unsigned char* line = &((const unsigned char*)data)[(h - 1 - y) * w * 4];

		for (int x = 0; x < w; ++x)
		{
			const int r = *line++;
			const int g = *line++;
			const int b = *line++;
			const int a = *line++;

			const int c = makeacol32(r, g, b, a);

			_putpixel32(bmp, x, y, c);
		}
	}

	return bmp;
}

#endif
