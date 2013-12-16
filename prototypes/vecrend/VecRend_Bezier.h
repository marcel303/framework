#pragma once

#include "Buffer.h"
#include "Types.h"

void VecRend_RenderBezier(Buffer* buffer, const PointF* points, int pointCount, float radius, float hardness);
void VecRend_SampleBezier(const PointF* points, int curveCount, std::vector<PointF>& out_Points);
