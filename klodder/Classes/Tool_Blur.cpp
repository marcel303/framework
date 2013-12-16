#include "Tool_Blur.h"

#if 0

void Tool_Blur::Apply(Dirty* dirty, ColBuf* buf, int x, int y, const Brush* brush, int size)
{
	Rect area;

	area.min[0] = x;
	area.min[1] = y;
	area.max[0] = x + brush->m_Sx - 1;
	area.max[1] = y + brush->m_Sy - 1;

	if (!area.Clip(buf->m_Area))
		return;

	for (int py = area.y1; py <= area.y2; ++py)
	{
		const Fix* bpix = &brush->m_Pixs[py - y][area.x1 - x];
		Col* tpix = buf->GetCol(area.x1, py);

		for (int px = area.x1; px <= area.x2; ++px, ++bpix, ++tpix)
		{
			Col sum(0, 0, 0);

//				int count = 0;
			const int count = (size * 2 + 1) * (size * 2 + 1);

			for (int oy = -size; oy <= +size; ++oy)
			{
				for (int ox = -size; ox <= +size; ++ox)
				{
					const Col* cpix = buf->GetCol_Safe(px + ox, py + oy);

//						if (!cpix)
//							continue;

					for (int i = 0; i < 3; ++i)
						sum.v[i] += cpix->v[i];

//						count++;
				}
			}

			sum.v[0] = sum.v[0] / count;
			sum.v[1] = sum.v[1] / count;
			sum.v[2] = sum.v[2] / count;

			const Fix t1 = FIX_INV(*bpix);
			const Fix t2 = FIX_INV(t1);

			for (int i = 0; i < 3; ++i)
				tpix->v[i] = FIX_MIX(tpix->v[i], sum.v[i], t1, t2);
		}
	}

	dirty->MakeDirty(&area);
}

#endif
