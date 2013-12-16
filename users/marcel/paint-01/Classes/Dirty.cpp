#include "dirty.h"
#include "log.h"

namespace Paint
{
	Dirty::Dirty()
	{
		for (int y = 0; y < COLBUF_SY; ++y)
		{
			m_Lines[y].dirty = false;
		}
	}

	void Dirty::Validate(int y)
	{
		DirtyEvent e;

		e.x1 = m_Lines[y].x1;
		e.x2 = m_Lines[y].x2;
		e.y = y;

		m_DirtyCB.Invoke(&e);

		//

		m_Lines[y].dirty = false;
	}

	void Dirty::Validate()
	{
		for (int y = 0; y < COLBUF_SY; ++y)
		{
			if (!m_Lines[y].dirty)
				continue;

			Validate(y);
		}
	}

	void Dirty::MakeDirty()
	{
		Rect rect(
			0,
			0,
			COLBUF_SX - 1,
			COLBUF_SY - 1); // fixme

		MakeDirty(&rect);
	}

	void Dirty::MakeDirty(const Rect* rect)
	{
		for (int y = rect->y1; y <= rect->y2; ++y)
		{
			MakeDirty(rect->x1, rect->x2, y);
		}
	}

	void Dirty::MakeDirty(int x1, int x2, int y)
	{
		DirtyLine* line = &m_Lines[y];

		if (line->dirty)
		{
			if (x2 < line->x1 || x1 > line->x2)
			{
				Validate(y);
			}
		}

		if (line->dirty)
		{
			if (x1 < line->x1)
				line->x1 = x1;
			if (x2 > line->x2)
				line->x2 = x2;
		}
		else
		{
			line->x1 = x1;
			line->x2 = x2;

			line->dirty = true;
		}
	}
};

