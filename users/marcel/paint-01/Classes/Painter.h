#pragma once

#include "brush.h"
#include "col.h"
#include "colbuf.h"
#include "coord.h"
#include "dirty.h"

namespace Paint
{
	void PaintBrush(Dirty* dirty, ColBuf* buf, Coord* coord, const Brush* brush, const Col* color, float opacity);
	void PaintBlur(Dirty* dirty, ColBuf* buf, int x, int y, const Brush* brush, int size);
	void PaintSmudge(Dirty* dirty, ColBuf* buf, int x, int y, int dx, int dy, const Brush* brush, Fix strength);
};
