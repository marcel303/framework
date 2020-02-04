#include "framework.h"
#include "nanovg.h"
#include "nanovg-framework.h"

class Canvas
{
public:
	enum ArcMode
	{
		kArcMode_Open,
		kArcMode_Chord,
		kArcMode_Pie,
	};
	
	enum EllipseMode
	{
		kEllipseMode_Radius,
		kEllipseMode_Corner
	};
	
	enum RectMode
	{
		kRectMode_Corner,  // (x, y). 3rd and 4th parameters are the width and height
		kRectMode_Corners, // absolute (x, y)'s
		kRectMode_Radius   // centered at (x, y). 3rd and 4th parameters are the (x, y) radius
	};
	
	enum StrokeCap
	{
		kStrokeCap_Butt,  // default mode. square but not projected
		kStrokeCap_Round, // rounded caps
		kStrokeCap_Square // makes caps appear square. similar to Processing's PROJECT mode
	};
	
	enum StrokeJoin
	{
		kStrokeJoin_Miter,
		kStrokeJoin_Round,
		kStrokeJoin_Bevel
	};
	
private:
	NVGcontext * ctx = nullptr;
	
	bool isFilled = true;
	bool isStroked = true;
	bool isFirstVertex = true;
	bool isDrawingShape = false;
	
	NVGcolor fillColor = nvgRGBA(255, 255, 255, 255);
	NVGcolor strokeColor = nvgRGBA(255, 255, 255, 255);
	
	EllipseMode ellipseMode_ = kEllipseMode_Radius;
	RectMode rectMode_ = kRectMode_Corner;
	
public:
	~Canvas()
	{
		shut();
	}
	
	void init()
	{
		Assert(ctx == nullptr);
		
		ctx = nvgCreateFramework(NVG_ANTIALIAS | NVG_STENCIL_STROKES | NVG_DITHER_GRADIENTS);
		
		nvgMiterLimit(ctx, 0.f);
	}
	
	void shut()
	{
		if (ctx != nullptr)
		{
			nvgDeleteFramework(ctx);
			ctx = nullptr;
		}
	}
	
	void begin()
	{
		if (ctx == nullptr)
			init();
		
		int sx;
		int sy;
		framework.getCurrentViewportSize(sx, sy);
		
		nvgBeginFrame(ctx, sx, sy, framework.getCurrentBackingScale());
	}
	
	void end()
	{
		nvgEndFrame(ctx);
	}
	
	void fill(int c)
	{
		fill(c, c, c);
	}
	
	void fill(int r, int g, int b, int a = 255)
	{
		isFilled = true;
		fillColor = nvgRGBA(r, g, b, a);
		if (isDrawingShape)
			nvgFillColor(ctx, fillColor);
	}
	
	void noFill()
	{
		isFilled = false;
	}
	
	void stroke(int c)
	{
		stroke(c, c, c);
	}
	
	void stroke(int r, int g, int b, int a = 255)
	{
		isStroked = true;
		strokeColor = nvgRGBA(r, g, b, a);
		if (isDrawingShape)
			nvgStrokeColor(ctx, strokeColor);
	}
	
	void noStroke()
	{
		isStroked = false;
	}
	
	void strokeWeight(float w)
	{
		nvgStrokeWidth(ctx, w);
	}
	
	void strokeCap(StrokeCap cap)
	{
		int cap_ =
			cap == kStrokeCap_Butt ? NVG_BUTT :
			cap == kStrokeCap_Round ? NVG_ROUND :
			cap == kStrokeCap_Square ? NVG_SQUARE :
			-1;
		
		if (cap_ != -1)
		{
			nvgLineCap(ctx, cap_);
		}
	}
	
	void strokeJoin(StrokeJoin join)
	{
		int join_ =
			join == kStrokeJoin_Miter ? NVG_MITER :
			join == kStrokeJoin_Round ? NVG_ROUND :
			join == kStrokeJoin_Bevel ? NVG_BEVEL :
			-1;
		
		if (join_ != -1)
		{
			nvgLineJoin(ctx, join_);
		}
	}
	
	void clip(float x, float y, float w, float h)
	{
		nvgScissor(ctx, x, y, w, h);
	}
	
	void noClip()
	{
		nvgResetScissor(ctx);
	}
	
	void beginShape()
	{
		nvgBeginPath(ctx);
		isFirstVertex = true;
		isDrawingShape = true;
		
		nvgStrokeColor(ctx, strokeColor);
		nvgFillColor(ctx, fillColor);
	}
	
	void endShape()
	{
		if (isFilled)
			nvgFill(ctx);
		if (isStroked)
			nvgStroke(ctx);
		isDrawingShape = false;
	}
	
	void moveTo(float x, float y)
	{
		nvgMoveTo(ctx, x, y);
	}
	
	void vertex(float x, float y)
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
	
	void arc(float x, float y, float r, float a1, float a2, ArcMode mode = kArcMode_Open)
	{
		if (mode == kArcMode_Open)
			nvgArc(ctx, x, y, r, a1, a2, NVG_CW);
		else if (mode == kArcMode_Pie)
		{
			nvgMoveTo(ctx, x, y);
			nvgArc(ctx, x, y, r, a1, a2, NVG_CW);
			nvgLineTo(ctx, x, y);
		}
		else if (mode == kArcMode_Chord)
		{
			float x1 = x + cosf(a1) * r;
			float y1 = y + sinf(a1) * r;
			
			nvgArc(ctx, x, y, r, a1, a2, NVG_CW);
			nvgLineTo(ctx, x1, y1);
		}
	}
	
	void circle(float x, float y, float r)
	{
		nvgCircle(ctx, x, y, r);
	}
	
	void ellipse(float x, float y, float rx, float ry)
	{
		if (ellipseMode_ == kEllipseMode_Radius)
			nvgEllipse(ctx, x, y, rx, ry);
		else if (ellipseMode_ == kEllipseMode_Corner)
			nvgEllipse(ctx, x + rx/2.f, y + ry/2.f, (rx - x) / 2.f, (ry - y) / 2.f);
	}
	
	void ellipseMode(EllipseMode mode)
	{
		ellipseMode_ = mode;
	}
	
	void line(float x1, float y1, float x2, float y2)
	{
		nvgMoveTo(ctx, x1, y1);
		nvgLineTo(ctx, x2, y2);
	}
	
	void lineTo(float x, float y)
	{
		nvgLineTo(ctx, x, y);
	}
	
	void rect(float x, float y, float w, float h)
	{
		if (rectMode_ == kRectMode_Corner)
			nvgRect(ctx, x, y, w, h);
		else if (rectMode_ == kRectMode_Corners)
			nvgRect(ctx, x, y, w - x, h - y);
		else if (rectMode_ == kRectMode_Radius)
			nvgRect(ctx, x - w, y - h, w * 2.f, h * 2.f);
	}
	
	void rect(float x, float y, float w, float h, float r)
	{
		if (rectMode_ == kRectMode_Corner)
			nvgRoundedRect(ctx, x, y, w, h, r);
		else if (rectMode_ == kRectMode_Corners)
			nvgRoundedRect(ctx, x, y, w - x, h - y, r);
		else if (rectMode_ == kRectMode_Radius)
			nvgRoundedRect(ctx, x - w, y - h, w * 2.f, h * 2.f, r);
	}
	
	void rectMode(RectMode mode)
	{
		rectMode_ = mode;
	}
	
	void square(float x, float y, float extent)
	{
		rect(x, y, extent, extent);
	}
	
	void triangle(float x1, float y1, float x2, float y2, float x3, float y3)
	{
		nvgMoveTo(ctx, x1, y1);
		nvgLineTo(ctx, x2, y2);
		nvgLineTo(ctx, x3, y3);
		nvgLineTo(ctx, x1, y1);
	}
	
	NVGcontext * getNanoVgContext() const { return ctx; }
};

namespace CanvasFunctions
{
	static Canvas canvas;
	
	inline void begin() { canvas.begin(); }
	inline void end() { canvas.end(); }
	
	inline void fill(int c) { canvas.fill(c); }
	inline void fill(int r, int g, int b, int a = 255) { canvas.fill(r, g, b, a); }
	inline void noFill() { canvas.noFill(); }
	
	inline void stroke(int c) { canvas.stroke(c); }
	inline void stroke(int r, int g, int b, int a = 255) { canvas.stroke(r, g, b, a); }
	inline void noStroke() { canvas.noStroke(); }
	
	inline void strokeWeight(float w) { canvas.strokeWeight(w); }
	inline void strokeCap(Canvas::StrokeCap cap) { canvas.strokeCap(cap); }
	inline void strokeJoin(Canvas::StrokeJoin join) { canvas.strokeJoin(join); }
	
	inline void clip(float x, float y, float w, float h) { canvas.clip(x, y, w, h); }
	inline void noClip() { canvas.noClip(); }
	
	inline void beginShape() { canvas.beginShape(); }
	inline void endShape() { canvas.endShape(); }
	
	inline void vertex(float x, float y) { canvas.vertex(x, y); }
	inline void moveTo(float x, float y) { canvas.moveTo(x, y); }
	
	inline void arc(float x, float y, float r, float a1, float a2, Canvas::ArcMode mode = Canvas::kArcMode_Open) { canvas.arc(x, y, r, a1, a2, mode); }
	inline void circle(float x, float y, float r) { canvas.circle(x, y, r); }
	inline void ellipse(float x, float y, float rx, float ry) { canvas.ellipse(x, y, rx, ry); }
	inline void ellipseMode(Canvas::EllipseMode mode) { canvas.ellipseMode(mode); }
	inline void line(float x1, float y1, float x2, float y2) { canvas.line(x1, y1, x2, y2); }
	inline void lineTo(float x, float y) { canvas.lineTo(x, y); }
	inline void rect(float x, float y, float w, float h) { canvas.rect(x, y, w, h); }
	inline void rect(float x, float y, float w, float h, float r) { canvas.rect(x, y, w, h, r); }
	inline void rectMode(Canvas::RectMode mode) { canvas.rectMode(mode); }
	inline void square(float x, float y, float extent) { canvas.square(x, y, extent); }
	inline void triangle(float x1, float y1, float x2, float y2, float x3, float y3) { canvas.triangle(x1, y1, x2, y2, x3, y3); }
}

using namespace CanvasFunctions;

int main(int argc, char * argv[])
{
	setupPaths(CHIBI_RESOURCE_PATHS);
	
	const int sx = 800;
	const int sy = 600;
	
	framework.enableDepthBuffer = true;
	
	if (!framework.init(sx, sy))
		return -1;
	
	float d = 71;

	for (;;)
	{
		framework.process();
		
		if (framework.quitRequested)
			break;
		
		float n = fmodf(framework.time, d);
		
		framework.beginDraw(255, 255, 255, 255);
		{
			begin();
			
			noFill();
			
			strokeJoin(Canvas::kStrokeJoin_Round);
			
			beginShape();
			{
				stroke(0, 0, 0);
				strokeWeight(.5f);
				
				for (int theta = 0; theta <= 360; ++theta)
				{
					const float k = theta * d * float(M_PI) / 180;
					const float r = 300 * sinf(int(n) * k);
					const float x = r * cosf(k) + sx/2;
					const float y = r * sinf(k) + sy/2;
					
					vertex(x, y);
				}
			}
			endShape();

			fill(63, 127, 255, 30);
			
			beginShape();
			{
				stroke(63, 127, 255, 63);
				strokeWeight(4.f);
				strokeJoin(Canvas::kStrokeJoin_Round);
				
				for (int theta = 0; theta <= 360; ++theta)
				{
					const float k = theta * float(M_PI) / 180;
					const float r = 300 * sinf(n * k);
					const float x = r * cosf(k) + sx/2;
					const float y = r * sinf(k) + sy/2;
					
					vertex(x, y);
				}
			}
			endShape();
			
			end();
		}
		framework.endDraw();
	}
	
	framework.shutdown();
	
	return 0;
}
