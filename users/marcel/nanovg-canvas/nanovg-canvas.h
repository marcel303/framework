#pragma once

struct NVGcontext;

class NvgCanvas
{
public:
	enum class ArcMode
	{
		Open,  // no connecting line is drawn between the starting and end points
		Chord, // a line connecting the starting and end points is added
		Pie    // lines are added connecting the starting and end points to the center of the arc
	};
	
	enum class EllipseMode
	{
		Radius, // centered at (x, y). 3rd and 4th parameters are the (x, y) radius
		Corner  // (x, y). 3rd and 4th parameters are the width and height
	};
	
	enum class RectMode
	{
		Corner,  // (x, y). 3rd and 4th parameters are the width and height
		Corners, // absolute (x, y)'s
		Radius   // centered at (x, y). 3rd and 4th parameters are the (x, y) radius
	};
	
	enum class StrokeCap
	{
		Butt,  // default mode. square but not projected
		Round, // rounded caps
		Square // makes caps appear square. similar to Processing's PROJECT mode
	};
	
	enum class StrokeJoin
	{
		Miter,
		Round,
		Bevel
	};
	
private:
	NVGcontext * ctx = nullptr;
	
	bool isFilled = true;
	bool isStroked = true;
	bool isFirstVertex = true;
	bool isDrawingShape = false;
	
	int fillColor[4] = { 255, 255, 255, 255 };
	int strokeColor[4] = { 255, 255, 255, 255 };
	
	EllipseMode _ellipseMode = EllipseMode::Radius;
	RectMode _rectMode = RectMode::Corner;
	
public:
	~NvgCanvas();
	
	void init();
	void shut();
	
	void begin();
	void end();
	
	void fill(int c);
	void fill(int r, int g, int b, int a = 255);
	void noFill();
	
	void stroke(int c);
	void stroke(int r, int g, int b, int a = 255);
	void noStroke();
	
	void strokeWeight(float w);
	void strokeCap(StrokeCap cap);
	void strokeJoin(StrokeJoin join);
	
	void clip(float x, float y, float w, float h);
	void noClip();
	
	void beginShape();
	void endShape();
	
	void moveTo(float x, float y);
	void vertex(float x, float y);
	
	void arc(float x, float y, float r, float a1, float a2, ArcMode mode = ArcMode::Open);
	void circle(float x, float y, float r);
	void ellipse(float x, float y, float rx, float ry);
	void ellipseMode(EllipseMode mode);
	void line(float x1, float y1, float x2, float y2);
	void lineTo(float x, float y);
	void rect(float x, float y, float w, float h);
	void rect(float x, float y, float w, float h, float r);
	void rectMode(RectMode mode);
	void square(float x, float y, float extent);
	void triangle(float x1, float y1, float x2, float y2, float x3, float y3);
	
	NVGcontext * getNanoVgContext() const { return ctx; }
};

namespace NvgCanvasFunctions
{
	using Canvas = NvgCanvas;
	
	using ArcMode = Canvas::ArcMode;
	using EllipseMode = Canvas::EllipseMode;
	using RectMode = Canvas::RectMode;
	using StrokeCap = Canvas::StrokeCap;
	using StrokeJoin = Canvas::StrokeJoin;
	
	//
	
	extern Canvas canvas;
	
	inline void begin() { canvas.begin(); }
	inline void end() { canvas.end(); }
	
	inline void fill(int c) { canvas.fill(c); }
	inline void fill(int r, int g, int b, int a = 255) { canvas.fill(r, g, b, a); }
	inline void noFill() { canvas.noFill(); }
	
	inline void stroke(int c) { canvas.stroke(c); }
	inline void stroke(int r, int g, int b, int a = 255) { canvas.stroke(r, g, b, a); }
	inline void noStroke() { canvas.noStroke(); }
	
	inline void strokeWeight(float w) { canvas.strokeWeight(w); }
	inline void strokeCap(StrokeCap cap) { canvas.strokeCap(cap); }
	inline void strokeJoin(StrokeJoin join) { canvas.strokeJoin(join); }
	
	inline void clip(float x, float y, float w, float h) { canvas.clip(x, y, w, h); }
	inline void noClip() { canvas.noClip(); }
	
	inline void beginShape() { canvas.beginShape(); }
	inline void endShape() { canvas.endShape(); }
	
	inline void vertex(float x, float y) { canvas.vertex(x, y); }
	inline void moveTo(float x, float y) { canvas.moveTo(x, y); }
	
	inline void arc(float x, float y, float r, float a1, float a2, ArcMode mode = ArcMode::Open) { canvas.arc(x, y, r, a1, a2, mode); }
	inline void circle(float x, float y, float r) { canvas.circle(x, y, r); }
	inline void ellipse(float x, float y, float rx, float ry) { canvas.ellipse(x, y, rx, ry); }
	inline void ellipseMode(EllipseMode mode) { canvas.ellipseMode(mode); }
	inline void line(float x1, float y1, float x2, float y2) { canvas.line(x1, y1, x2, y2); }
	inline void lineTo(float x, float y) { canvas.lineTo(x, y); }
	inline void rect(float x, float y, float w, float h) { canvas.rect(x, y, w, h); }
	inline void rect(float x, float y, float w, float h, float r) { canvas.rect(x, y, w, h, r); }
	inline void rectMode(RectMode mode) { canvas.rectMode(mode); }
	inline void square(float x, float y, float extent) { canvas.square(x, y, extent); }
	inline void triangle(float x1, float y1, float x2, float y2, float x3, float y3) { canvas.triangle(x1, y1, x2, y2, x3, y3); }
}
