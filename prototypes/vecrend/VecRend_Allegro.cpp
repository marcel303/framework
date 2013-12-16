//#include "vecview_pch.h"
#include "VecRend_Allegro.h"

BITMAP* VecRend_ToBitmap(const Buffer* buffer)
{
	BITMAP* bmp = create_bitmap_ex(32, buffer->m_Sx, buffer->m_Sy);

	for (int y = 0; y < bmp->h; ++y)
	{
		const float* sline = buffer->GetLine(y);
		int* dline = (int*)bmp->line[y];

		for (int x = 0; x < bmp->w; ++x)
		{
			const float r = sline[0] * 255.0f * sline[3];
			const float g = sline[1] * 255.0f * sline[3];
			const float b = sline[2] * 255.0f * sline[3];
			const float a = sline[3] * 255.0f;

			dline[x] = makeacol32((int)r, (int)g, (int)b, (int)a);

			sline += 4;
		}
	}

	return bmp;
}
