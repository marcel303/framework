#include "Precompiled.h"
#include "Image_Allegro.h"
#include "Preview.h"

#ifndef NO_ALLEGRO

BITMAP* Atlas_ToBitmap(const Atlas* atlas)
{
	BITMAP* result = Image_ToBitmap(atlas->m_Texture);

#if 0
	for (int y = 0; y < builder->m_FillSy; ++y)
	{
		const int* line = builder->GetLine(y);

		for (int x = 0; x < builder->m_FillSx; ++x)
		{
			const int x1 = (x + 0) * builder->m_Interval - 0;
			const int y1 = (y + 0) * builder->m_Interval - 0;
			const int x2 = (x + 1) * builder->m_Interval - 1;
			const int y2 = (y + 1) * builder->m_Interval - 1;

			const int filled = line[x];

			int c;

			if (!filled)
			{
				c = makecol(0, 0, 127);

				rectfill(result, x1, y1, x2, y2, c);
			}
		}
	}
#endif

#if 0
	for (size_t i = 0; i < atlas->m_Images.size(); ++i)
	{
		const Atlas_ImageInfo* image = atlas->m_Images[i];

		const int x = image->m_Position[0] + image->m_Offset[0];
		const int y = image->m_Position[1] + image->m_Offset[1];

		rect(result,
			x,
			y,
			x + image->m_ImageSize[0] - 1,
			y + image->m_ImageSize[1] - 1,
			makecol(0, 255, 0));

		textprintf_ex(result, font, x + 2, y + 2, makecol(0, 255, 0), -1, image->m_FileName.c_str());
	}
#endif

	return result;
}

#endif

