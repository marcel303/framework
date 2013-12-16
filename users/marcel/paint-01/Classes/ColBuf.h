#pragma once

#include "Brush.h"
#include "Col.h"
#include "Rect.h"

#define COLBUF_SX 320
#define COLBUF_SY 480

namespace Paint
{
	class Dirty;

	class ColBuf
	{
	public:
		ColBuf();

		void Clear();
		void ClearToColor(Col col);

		int GetColAA(Fix x, Fix y, const Col** out_pix, Fix* out_w) const;
		void GetColAA(Fix x, Fix y, Col* out_pix) const;

		void ApplyPaint(Dirty* dirty, int x, int y, const Brush* brush, const Col* col, Fix opacity);
		void ApplySmoothe(Dirty* dirty, int x, int y, const Brush* brush, int size);
		void ApplySmudge(Dirty* dirty, int x, int y, int dx, int dy, const Brush* brush, Fix strength);

		inline Col* GetCol(int x, int y)
		{
			return m_Lines[y] + x;
		}

		inline const Col* GetCol(int x, int y) const
		{
			return m_Lines[y] + x;
		}

		inline Col* DoGetCol_Safe(int x, int y)
		{
#if 0
			// Return border color.
			
			if (x < 0 || y < 0 || x >= COLBUF_SX || y >= COLBUF_SY)
				//return 0;
				return &m_DummyCol;
#else
			// Return edge color.
			
			if (x < 0)
				x = 0;
			if (y < 0)
				y = 0;
			if (x > COLBUF_SX - 1)
				x = COLBUF_SX - 1;
			if (y > COLBUF_SY - 1)
				y = COLBUF_SY - 1;
#endif

			return GetCol(x, y);
		}

		inline Col* GetCol_Safe(int x, int y)
		{
			return DoGetCol_Safe(x, y);
		}
		
		inline const Col* GetCol_Safe(int x, int y) const
		{
			ColBuf* self = (ColBuf*)this;
			
			return self->DoGetCol_Safe(x, y);
		}

		inline const Col* GetCol_Safe2(int x, int y) const
		{
			if (x < 0 || y < 0 || x >= COLBUF_SX || y >= COLBUF_SY)
				return 0;

			return GetCol(x, y);
		}

		Col m_Lines[COLBUF_SY][COLBUF_SX];
		Rect m_Area;
		Col m_EdgeCol;
		Col m_DummyCol;
	};
};
