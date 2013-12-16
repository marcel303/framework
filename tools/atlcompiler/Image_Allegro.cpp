#include "Precompiled.h"
#include "Image_Allegro.h"

#ifndef NO_ALLEGRO

BITMAP* Image_ToBitmap(const Image* image)
{
	BITMAP* result = create_bitmap_ex(32, image->m_Sx, image->m_Sy);

	for (int y = 0; y < image->m_Sy; ++y)
	{
		const ImagePixel* sline = image->GetLine(y);
		unsigned int* dline = (unsigned int*)result->line[y];

		for (int x = 0; x < image->m_Sx; ++x)
		{
			const int c = makeacol32(sline->r, sline->g, sline->b, sline->a);

			*dline = c;

			sline++;
			dline++;
		}
	}

	return result;
}

#endif

