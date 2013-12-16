#include <allegro.h>
#include "MaskMap.h"

BOOL MaskMap::Load(char* fileName)
{
	BITMAP* bmp = load_bitmap(fileName, 0);

	if (!bmp)
		return FALSE;

	SetSize(bmp->w, bmp->h);

	for (int x = 0; x < m_Sx; ++x)
	{
		for (int y = 0; y < m_Sy; ++y)
		{
			int c = getpixel(bmp, x, y);

			int r = getr(c);
			int g = getg(c);
			int b = getb(c);

			c = r + g + b;

			if (c)
				c = 1;
			else
				c = 0;

			m_Values[x + y * m_Sx] = c;
		}
	}

	destroy_bitmap(bmp);

	return TRUE;
}
