#include "ColBuf.h"
#include "Dirty.h"
#include "Painter.h"

namespace Paint
{
	ColBuf::ColBuf()
	{
		m_Area.x1 = 0;
		m_Area.y1 = 0;
		m_Area.x2 = COLBUF_SX - 1;
		m_Area.y2 = COLBUF_SY - 1;

		m_DummyCol.v[0] = 0;
		m_DummyCol.v[1] = 0;
		m_DummyCol.v[2] = 0;

		Clear();
	}

	void ColBuf::Clear()
	{
		Col col;

		col.v[0] = INT_TO_FIX(127);
		col.v[1] = INT_TO_FIX(127);
		col.v[2] = INT_TO_FIX(127);

		ClearToColor(col);
	}

	void ColBuf::ClearToColor(Col col)
	{
		for (int y = 0; y < COLBUF_SY; ++y)
		{
			Col* line = m_Lines[y];
			
			for (int x = 0; x < COLBUF_SX; ++x)
				line[x] = col;
		}
	}

	int ColBuf::GetColAA(Fix x, Fix y, const Col** out_pix, Fix* out_w) const
	{
		const int ix1 = FIX_TO_INT(x);
		const int iy1 = FIX_TO_INT(y);
		const int ix2 = ix1 + 1;
		const int iy2 = iy1 + 1;

		int correct = ix1 < 0 || iy1 < 0 || ix2 >= COLBUF_SX || iy2 >= COLBUF_SY;

		if (!correct)
		{
			out_pix[0] = GetCol(ix1, iy1);
			out_pix[1] = GetCol(ix2, iy1);
			out_pix[2] = GetCol(ix1, iy2);
			out_pix[3] = GetCol(ix2, iy2);
		}
		else
		{
			out_pix[0] = GetCol_Safe(ix1, iy1);
			out_pix[1] = GetCol_Safe(ix2, iy1);
			out_pix[2] = GetCol_Safe(ix1, iy2);
			out_pix[3] = GetCol_Safe(ix2, iy2);
		}

		const Fix wx2 = x - INT_TO_FIX(ix1);
		const Fix wy2 = y - INT_TO_FIX(iy1);
		const Fix wx1 = FIX_INV(wx2);
		const Fix wy1 = FIX_INV(wy2);

		out_w[0] = FIX_MUL(wx1, wy1);
		out_w[1] = FIX_MUL(wx2, wy1);
		out_w[2] = FIX_MUL(wx1, wy2);
		out_w[3] = FIX_MUL(wx2, wy2);

		return correct;
	}

	void ColBuf::GetColAA(Fix x, Fix y, Col* out_pix) const
	{
		const Col* pix[4];
		Fix w[4];

		/*int correct = */GetColAA(x, y, pix, w);

		for (int i = 0; i < 3; ++i)
			out_pix->v[i] = 0;

		//if (!correct)
		//{
			for (int j = 0; j < 4; ++j)
			{
				out_pix->v[0] += FIX_MUL(pix[j]->v[0], w[j]);
				out_pix->v[1] += FIX_MUL(pix[j]->v[1], w[j]);
				out_pix->v[2] += FIX_MUL(pix[j]->v[2], w[j]);
			}
		/*}
		else
		{
			for (int j = 0; j < 4; ++j)
			{
				if (!pix[j])
					continue;
			
				out_pix->v[0] += FIX_MUL(pix[j]->v[0], w[j]);
				out_pix->v[1] += FIX_MUL(pix[j]->v[1], w[j]);
				out_pix->v[2] += FIX_MUL(pix[j]->v[2], w[j]);
			}
		}*/
	}

	void ColBuf::ApplyPaint(Dirty* dirty, int x, int y, const Brush* brush, const Col* col, Fix opacity)
	{
		x -= brush->m_OffsetX;
		y -= brush->m_OffsetY;

		Rect area;

		area.x1 = x;
		area.y1 = y;
		area.x2 = x + brush->m_Sx - 1;
		area.y2 = y + brush->m_Sy - 1;

		if (!area.Clip(m_Area))
			return;

		//

		const Col* dstCol = col;

		for (int py = area.y1; py <= area.y2; ++py)
		{
			Col* srcCol = GetCol(area.x1, py);
			const Fix* dstPix = &brush->m_Pixs[py - y][area.x1 - x];

			for (int px = area.x1; px <= area.x2; ++px)
			{
				const Fix t1 = FIX_MUL(*dstPix, opacity);
				const Fix t2 = FIX_INV(t1);

				for (int i = 0; i < 3; ++i)
				{
					srcCol->v[i] = FIX_MIX(
						srcCol->v[i],
						dstCol->v[i],
						t2, t1);
				}

				++srcCol;
				++dstPix;
			}
		}

		dirty->MakeDirty(&area);
	}

	void ColBuf::ApplySmoothe(Dirty* dirty, int x, int y, const Brush* brush, int size)
	{
		x -= brush->m_OffsetX;
		y -= brush->m_OffsetY;

		PaintBlur(dirty, this, x, y, brush, size);
	}

	void ColBuf::ApplySmudge(Dirty* dirty, int x, int y, int dx, int dy, const Brush* brush, Fix strength)
	{
		x -= brush->m_OffsetX;
		y -= brush->m_OffsetY;

		PaintSmudge(dirty, this, x, y, dx, dy, brush, strength);
	}
};
