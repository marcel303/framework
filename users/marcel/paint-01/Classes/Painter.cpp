#include <string.h>
#include "Painter.h"

#define TEMPBUF_SX BRUSH_MAXSX
#define TEMPBUF_SY BRUSH_MAXSY

namespace Paint
{
	// Temporary storage capacity for drawing operations.
	
	static Col s_TempBuf[TEMPBUF_SY][TEMPBUF_SX];

	static void TempBuf_Init(ColBuf* buf, int x1, int y1, int x2, int y2);
	static void TempBuf_Clear(int sx, int sy);
	static void TempBuf_ClearToColor(int sx, int sy, const Col* col);
	
	#if 0
	void PaintBrush(Dirty* dirty, ColBuf* buf, Coord* coord, const Brush* brush, const Col* color, float opacity)
	{
		Rect area;

		area.min[0] = FL(coord->p[0]);
		area.min[1] = FL(coord->p[1]);
		area.max[0] = CL(coord->p[0] + brush->m_Sx);
		area.max[1] = CL(coord->p[1] + brush->m_Sy);

		float minf[2];

		minf[0] = coord->p[0];
		minf[1] = coord->p[1];

		for (int y = 0; y < brush->m_Sy; ++y)
		{
			//for (int x = 0; x < brush->m_Sx; ++x)
			{
				buf->Draw(
					minf[0],
					minf[0] + brush->m_Sx - 1,
					minf[1] + y,
					&brush->m_Pixs[y][0],
					color,
					opacity);
			}
		}

		dirty->MakeDirty(&area);
	}
	#endif

	void PaintBlur(Dirty* dirty, ColBuf* buf, int x, int y, const Brush* brush, int size)
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

	void PaintSmudge(Dirty* dirty, ColBuf* buf, int x, int y, int dx, int dy, const Brush* brush, Fix strength)
	{
		Rect area;

		area.min[0] = x;
		area.min[1] = y;
		area.max[0] = x + brush->m_Sx - 1;
		area.max[1] = y + brush->m_Sy - 1;

		for (int py = 0; py < brush->m_Sy; ++py)
		{
			const Fix* bpix = brush->m_Pixs[py];
			Col* cpix1 = &s_TempBuf[py][0];

			for (int px = 0; px < brush->m_Sx; ++px)
			{
#if 0
				const Fix s = FIX_MUL(*bpix, strength);

				const Fix sx = INT_TO_FIX(x + px) - s * dx;
				const Fix sy = INT_TO_FIX(y + py) - s * dy;
#else
				const Fix sx = INT_TO_FIX(x + px) - FIX_MUL(FIX_MUL(*bpix, dx), strength);
				const Fix sy = INT_TO_FIX(y + py) - FIX_MUL(FIX_MUL(*bpix, dy), strength);
#endif

				buf->GetColAA(sx, sy, cpix1);
				
				bpix++;
				cpix1++;
			}
		}

#if 0
		buf->m_DummyCol.v[0] = 0; // fixme
		buf->m_DummyCol.v[1] = 0;
		buf->m_DummyCol.v[2] = 0;
#endif
		
		if (!area.Clip(buf->m_Area))
			return;

#if 1
		const int sx = (area.x2 - area.x1 + 1) * 3;

		for (int py = area.y1; py <= area.y2; ++py)
		{
			const Col* tpix = &s_TempBuf[py - y][area.x1 - x];
			Col* cpix = buf->GetCol(area.x1, py);

#if 0
			const Fix* src = (Fix*)tpix;
			Fix* dst = (Fix*)cpix;
			
			for (int i = 0; i < sx; ++i)
				*dst++ = *src++;
#else
			memcpy(cpix, tpix, sizeof(Fix) * sx);
#endif

			tpix += sx;
			cpix += sx;
		}
#endif

		dirty->MakeDirty(&area);
	}

	//
	
	static void TempBuf_Init(const ColBuf* buf, int x1, int y1, int x2, int y2)
	{
		for (int y = y1; y <= y1; ++y)
		{
			const Col* src = buf->m_Lines[y];
			Col* dst = s_TempBuf[y - y1];

			for (int x = x1; x <= x2; ++x)
			{
				*dst++ = *src++;
			}
		}
	}

	static void TempBuf_Clear(int sx, int sy)
	{
		Col col;

		col.v[0] = 0;
		col.v[1] = 0;
		col.v[2] = 0;

		for (int y = 0; y < sy; ++y)
		{
			Col* line = s_TempBuf[y];

			for (int x = 0; x < sx; ++x, ++line)
				*line = col;
		}
	}

	static void TempBuf_ClearToColor(int sx, int sy, const Col* col)
	{
		for (int y = 0; y < sy; ++y)
		{
			Col* line = s_TempBuf[y];

			for (int x = 0; x < sx; ++x, ++line)
				*line = *col;
		}
	}
};
