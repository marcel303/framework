#include "Precompiled.h"
#include "VecRend_Bezier.h"

// todo: draw soft dots using brush
// todo: clip brush rect

static inline void RenderPixel(Buffer* buffer, int x, int y, float v)
{
	//if (x < 0 || y < 0 || x >= buffer->m_Sx || y >= buffer->m_Sy)
	//	return;

	float* line = buffer->GetLine(y) + x * 4;

	for (int i = 0; i < 4; ++i)
	{
		if (v > line[i])
			line[i] = v;
	}
}

static void RenderDot(Buffer* buffer, float x, float y, float radius, float hardness)
{
	int x1 = (int)floorf(x - radius);
	int y1 = (int)floorf(y - radius);
	int x2 = (int)ceilf(x + radius);
	int y2 = (int)ceilf(y + radius);

	if (x1 < 0)
		x1 = 0;
	if (y1 < 0)
		y1 = 0;
	if (x2 > buffer->m_Sx - 1)
		x2 = buffer->m_Sx - 1;
	if (y2 > buffer->m_Sy - 1)
		y2 = buffer->m_Sy - 1;

	const float radiusRcp = 1.0f / radius;

	for (int py = y1; py <= y2; ++py)
	{
		const float dy = py - y;
		const float dy2 = dy * dy;

		for (int px = x1; px <= x2; ++px)
		{
			const float dx = px - x;
			const float dx2 = dx * dx;

			float v = sqrtf(dx2 + dy2) * radiusRcp;
			
			v = powf(v, hardness);
			
			float c = 1.0f - v;
			
			if (c < 0.0f)
				c = 0.0f;
			else if (c > 1.0f)
				c = 1.0f;

			RenderPixel(buffer, px, py, c);
		}
	}
}

static PointF Evaluate(const PointF* points, float t)
{
	// (1-t)^1*p1 + (1-t)^2*p2 + t^2*p3 + t^1*p4

	const float t1 = 1.0f - t;
	const float t2 = t;

	const float x =
		1.0f * points[0].x * t1 * t1 * t1 +
		3.0f * points[1].x * t1 * t1 * t2 +
		3.0f * points[2].x * t2 * t2 * t1 +
		1.0f * points[3].x * t2 * t2 * t2;

	const float y =
		1.0f * points[0].y * t1 * t1 * t1 +
		3.0f * points[1].y * t1 * t1 * t2 +
		3.0f * points[2].y * t2 * t2 * t1 +
		1.0f * points[3].y * t2 * t2 * t2;

	return PointF(x, y);
}

static void SamplePoints(const PointF* points, float t1, float t2, std::vector<float>& out_T)
{
	const PointF p1 = Evaluate(points, t1);
	const PointF p2 = Evaluate(points, t2);

	const float distanceSq = p1.DistanceSq(p2);

	//const float treshold = 0.5f;
	const float treshold = 1.0f;
	const float tresholdSq = treshold * treshold;

	if (distanceSq >= tresholdSq)
	{
		const float t = (t1 + t2) / 2.0f;

		SamplePoints(points, t1, t, out_T);
		SamplePoints(points, t, t2, out_T);
	}
	else
	{
		const float t = (t1 + t2) / 2.0f;

		out_T.push_back(t);
	}
}

static void RenderSegment(Buffer* buffer, const PointF* points, float radius, float hardness)
{
	std::vector<float> t;

	SamplePoints(points, 0.0f, 0.5f, t);
	SamplePoints(points, 0.5f, 1.0f, t);

	for (size_t i = 0; i < t.size(); ++i)
	{
		PointF point = Evaluate(points, t[i]);

		RenderDot(buffer, point.x, point.y, radius, hardness);
	}
}

void VecRend_RenderBezier(Buffer* buffer, const PointF* points, int curveCount, float radius, float hardness)
{
	for (int i = 0; i < curveCount; ++i)
	{
		RenderSegment(buffer, points + i * 4, radius, hardness);
	}
}

void VecRend_SampleBezier(const PointF* _points, int curveCount, std::vector<PointF>& out_Points)
{
	for (int i = 0; i < curveCount; ++i)
	{
		const PointF* points = _points + i * 4;

		std::vector<float> t;

		SamplePoints(points, 0.0f, 0.5f, t);
		SamplePoints(points, 0.5f, 1.0f, t);

		for (size_t j = 0; j < t.size(); ++j)
		{
			PointF point = Evaluate(points, t[j]);

			out_Points.push_back(point);
		}
	}
}
