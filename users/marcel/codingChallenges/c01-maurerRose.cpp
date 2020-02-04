#include "framework.h"
#include "nanovg.h"
#include "nanovg-framework.h"

class Canvas
{
	NVGcontext * ctx = nullptr;
	
	bool isFilled = true;
	bool isFirstVertex = true;
	bool isDrawingShape = false;
	
	NVGcolor fillColor = nvgRGBA(255, 255, 255, 255);
	NVGcolor strokeColor = nvgRGBA(255, 255, 255, 255);
	
public:
	~Canvas()
	{
		shut();
	}
	
	void init()
	{
		Assert(ctx == nullptr);
		
		ctx = nvgCreateFramework(NVG_ANTIALIAS | NVG_STENCIL_STROKES | NVG_DITHER_GRADIENTS);
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
		strokeColor = nvgRGBA(r, g, b, a);
		if (isDrawingShape)
			nvgStrokeColor(ctx, strokeColor);
	}
	
	void strokeWeight(float w)
	{
		nvgStrokeWidth(ctx, w);
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
		//else
			nvgStroke(ctx);
		isDrawingShape = false;
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
};

int main(int argc, char * argv[])
{
	setupPaths(CHIBI_RESOURCE_PATHS);
	
	const int sx = 800;
	const int sy = 600;
	
	framework.enableDepthBuffer = true;
	
	if (!framework.init(sx, sy))
		return -1;
	
	Canvas canvas;
	
	float d = 71;

	for (;;)
	{
		framework.process();
		
		if (framework.quitRequested)
			break;
		
		float n = fmodf(framework.time, d);
		
		framework.beginDraw(255, 255, 255, 255);
		{
			canvas.begin();
			
			canvas.noFill();
			
			canvas.beginShape();
			{
				canvas.stroke(0, 0, 0);
				canvas.strokeWeight(.5f);
				
				for (int theta = 0; theta <= 360; ++theta)
				{
					const float k = theta * d * float(M_PI) / 180;
					const float r = 300 * sinf(int(n) * k);
					const float x = r * cosf(k) + sx/2;
					const float y = r * sinf(k) + sy/2;
					
					canvas.vertex(x, y);
				}
			}
			canvas.endShape();

			canvas.fill(63, 127, 255, 30);
			
			canvas.beginShape();
			{
				canvas.stroke(255, 0, 0);
				canvas.strokeWeight(4.f);
				
				for (int theta = 0; theta <= 360; ++theta)
				{
					const float k = theta * float(M_PI) / 180;
					const float r = 300 * sinf(n * k);
					const float x = r * cosf(k) + sx/2;
					const float y = r * sinf(k) + sy/2;
					
					canvas.vertex(x, y);
				}
			}
			canvas.endShape();
			
			canvas.end();
		}
		framework.endDraw();
	}
	
	canvas.shut();
	
	framework.shutdown();
	
	return 0;
}
