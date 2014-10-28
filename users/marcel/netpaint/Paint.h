#pragma once

#include "Brush.h"
#include "Canvas.h"
#include "Coord.h"
#include "Validatable.h"

void PaintBrush(Validatable* val, Canvas* canvas, coord_t* coord, Brush* brush, float* color, float opacity);
void PaintBlur(Validatable* val, Canvas* canvas, coord_t* coord, Brush* brush, int size);
void PaintSmudge(Validatable* val, Canvas* canvas, coord_t* coord, Brush* brush, coord_t* direction, float strength);
