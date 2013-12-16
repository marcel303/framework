#if 0

#if 0
#include <allegro.h>
#endif

#include "MaskMap.h"

XBOOL MaskMap::Load(char* fileName)
{
#if 0
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
#else
	return XFALSE;
#endif

	return XTRUE;
}

#endif
