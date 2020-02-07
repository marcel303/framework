#include "Debugging.h"
#include "framework.h"
#include "nanovg.h"
#include "nanovg-canvas.h"
#include "nanovg-framework.h"

namespace NvgCanvasFunctions
{
	Canvas canvas;
}

NvgCanvas::~NvgCanvas()
{
	shut();
}

void NvgCanvas::init()
{
	Assert(ctx == nullptr);
	
	ctx = nvgCreateFramework(NVG_ANTIALIAS | NVG_STENCIL_STROKES | NVG_DITHER_GRADIENTS);
}

void NvgCanvas::shut()
{
	if (ctx != nullptr)
	{
		nvgDeleteFramework(ctx);
		ctx = nullptr;
	}
}

void NvgCanvas::begin()
{
	if (ctx == nullptr)
		init();
	
	int sx;
	int sy;
	framework.getCurrentViewportSize(sx, sy);
	
	nvgBeginFrame(ctx, sx, sy, framework.getCurrentBackingScale());
}

void NvgCanvas::end()
{
	nvgEndFrame(ctx);
}

void NvgCanvas::fill(int c)
{
	fill(c, c, c);
}

void NvgCanvas::fill(int r, int g, int b, int a)
{
	isFilled = true;
	fillColor[0] = r;
	fillColor[1] = g;
	fillColor[2] = b;
	fillColor[3] = a;
	if (isDrawingShape)
		nvgFillColor(ctx, nvgRGBA(r, g, b, a));
}

void NvgCanvas::noFill()
{
	isFilled = false;
}

void NvgCanvas::stroke(int c)
{
	stroke(c, c, c);
}

void NvgCanvas::stroke(int r, int g, int b, int a)
{
	isStroked = true;
	strokeColor[0] = r;
	strokeColor[1] = g;
	strokeColor[2] = b;
	strokeColor[3] = a;
	if (isDrawingShape)
		nvgStrokeColor(ctx, nvgRGBA(r, g, b, a));
}

void NvgCanvas::noStroke()
{
	isStroked = false;
}

void NvgCanvas::strokeWeight(float w)
{
	nvgStrokeWidth(ctx, w);
}

void NvgCanvas::strokeCap(StrokeCap cap)
{
	int cap_ =
		cap == StrokeCap::Butt ? NVG_BUTT :
		cap == StrokeCap::Round ? NVG_ROUND :
		cap == StrokeCap::Square ? NVG_SQUARE :
		-1;
	
	if (cap_ != -1)
	{
		nvgLineCap(ctx, cap_);
	}
}

void NvgCanvas::strokeJoin(StrokeJoin join)
{
	int join_ =
		join == StrokeJoin::Miter ? NVG_MITER :
		join == StrokeJoin::Round ? NVG_ROUND :
		join == StrokeJoin::Bevel ? NVG_BEVEL :
		-1;
	
	if (join_ != -1)
	{
		nvgLineJoin(ctx, join_);
	}
}

void NvgCanvas::clip(float x, float y, float w, float h)
{
	nvgScissor(ctx, x, y, w, h);
}

void NvgCanvas::noClip()
{
	nvgResetScissor(ctx);
}

void NvgCanvas::beginShape()
{
	nvgBeginPath(ctx);
	isFirstVertex = true;
	isDrawingShape = true;
	
	nvgStrokeColor(ctx, nvgRGBA(strokeColor[0], strokeColor[1], strokeColor[2], strokeColor[3]));
	nvgFillColor(ctx, nvgRGBA(fillColor[0], fillColor[1], fillColor[2], fillColor[3]));
}

void NvgCanvas::endShape()
{
	if (isFilled)
		nvgFill(ctx);
	if (isStroked)
		nvgStroke(ctx);
	isDrawingShape = false;
}

void NvgCanvas::moveTo(float x, float y)
{
	nvgMoveTo(ctx, x, y);
}

void NvgCanvas::vertex(float x, float y)
{
	if (isFirstVertex)
	{
		isFirstVertex = false;
		nvgMoveTo(ctx, x, y);
	}
	else
	{
		nvgLineTo(ctx, x, y);
	}
}

void NvgCanvas::arc(float x, float y, float r, float a1, float a2, ArcMode mode)
{
	if (mode == ArcMode::Open)
	{
		nvgArc(ctx, x, y, r, a1, a2, NVG_CW);
	}
	else if (mode == ArcMode::Pie)
	{
		nvgMoveTo(ctx, x, y);
		nvgArc(ctx, x, y, r, a1, a2, NVG_CW);
		nvgLineTo(ctx, x, y);
	}
	else if (mode == ArcMode::Chord)
	{
		float x1 = x + cosf(a1) * r;
		float y1 = y + sinf(a1) * r;
		
		nvgArc(ctx, x, y, r, a1, a2, NVG_CW);
		nvgLineTo(ctx, x1, y1);
	}
}

void NvgCanvas::circle(float x, float y, float r)
{
	nvgCircle(ctx, x, y, r);
}

void NvgCanvas::ellipse(float x, float y, float rx, float ry)
{
	if (_ellipseMode == EllipseMode::Radius)
	{
		nvgEllipse(ctx, x, y, rx, ry);
	}
	else if (_ellipseMode == EllipseMode::Corner)
	{
		nvgEllipse(ctx, x + rx/2.f, y + ry/2.f, (rx - x) / 2.f, (ry - y) / 2.f);
	}
}

void NvgCanvas::ellipseMode(EllipseMode mode)
{
	_ellipseMode = mode;
}

void NvgCanvas::line(float x1, float y1, float x2, float y2)
{
	nvgMoveTo(ctx, x1, y1);
	nvgLineTo(ctx, x2, y2);
}

void NvgCanvas::lineTo(float x, float y)
{
	nvgLineTo(ctx, x, y);
}

void NvgCanvas::rect(float x, float y, float w, float h)
{
	if (_rectMode == RectMode::Corner)
	{
		nvgRect(ctx, x, y, w, h);
	}
	else if (_rectMode == RectMode::Corners)
	{
		nvgRect(ctx, x, y, w - x, h - y);
	}
	else if (_rectMode == RectMode::Radius)
	{
		nvgRect(ctx, x - w, y - h, w * 2.f, h * 2.f);
	}
}

void NvgCanvas::rect(float x, float y, float w, float h, float r)
{
	if (_rectMode == RectMode::Corner)
	{
		nvgRoundedRect(ctx, x, y, w, h, r);
	}
	else if (_rectMode == RectMode::Corners)
	{
		nvgRoundedRect(ctx, x, y, w - x, h - y, r);
	}
	else if (_rectMode == RectMode::Radius)
	{
		nvgRoundedRect(ctx, x - w, y - h, w * 2.f, h * 2.f, r);
	}
}

void NvgCanvas::rectMode(RectMode mode)
{
	_rectMode = mode;
}

void NvgCanvas::square(float x, float y, float extent)
{
	rect(x, y, extent, extent);
}

void NvgCanvas::triangle(float x1, float y1, float x2, float y2, float x3, float y3)
{
	nvgMoveTo(ctx, x1, y1);
	nvgLineTo(ctx, x2, y2);
	nvgLineTo(ctx, x3, y3);
	nvgLineTo(ctx, x1, y1);
}
