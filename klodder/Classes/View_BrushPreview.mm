#import "AppDelegate.h"
#import "Application.h"
#import "Benchmark.h"
#import "Bezier.h"
#import "Brush_Pattern.h"
#import "BrushSettingsLibrary.h"
#import "ExceptionLoggerObjC.h"
#import "MacImage.h"
#import "TravellerCapturer.h"
#import "View_BrushPreview.h"
#import "View_ColorPickerMgr.h"

#import "Calc.h"
#import "LayerMgr.h" // fixme, checkerboard

@implementation View_BrushPreview

-(id)initWithFrame:(CGRect)frame app:(AppDelegate*)_app settings:(BrushSettings*)_settings type:(ToolViewType)_type color:(Rgba)_color
{
	HandleExceptionObjcBegin();
	
    if ((self = [super initWithFrame:frame])) 
	{
		[self setOpaque:FALSE];
		[self setBackgroundColor:[UIColor clearColor]];
		
		app = _app;
		settings = _settings;
		type = _type;
		color = _color;
		patternId = 0;
		brushPreview = new MacImage();
		
		[self setNeedsDisplay];
    }
	
    return self;
	
	HandleExceptionObjcEnd(false);
	
	return nil;
}

-(void)redraw
{
	Benchmark bm("View_BrushPreview: redraw");
	
	const float scale = [AppDelegate displayScale];
	const float opacity = app.colorPickerState->Opacity_get();
	
	Assert(opacity >= 0.0f && opacity <= 1.0f);
	
	settings->Validate();
	
	const int sx = self.frame.size.width * scale;
	const int sy = self.frame.size.height * scale;
	const float border = (settings->diameter - 1) / 2.0f;
	
	TravellerCapturer capturer;
	
	std::vector<Vec2F> points;
	
	UsingBegin(Benchmark bm2("View_BrushPreview: redraw: calculate path"));
	{
		BezierNode nodes[] =
		{
			BezierNode(Vec2F(border, border), Vec2F(-100.0f * scale, 0.0f), Vec2F(100.0f * scale, 0.0f)),
			BezierNode(Vec2F(sx - border - 1.0f, sy - border - 1.0f), Vec2F(-100.0f * scale, 0.0f), Vec2F(100.0f * scale, 0.0f)),
		};
		
		BezierPath path;
		
		path.ConstructFromNodes(nodes, 2);

		points = path.Sample(50, false);
		
		float distance = settings->diameter * settings->spacing;
		
		if (distance < 0.5f)
			distance = 0.5f;
		
		points = capturer.Capture(distance, points);
		
		LOG_DBG("pointCount: %lu", points.size());
	}
	UsingEnd();
	
	Filter filter;
	
	filter.Size_set(sx, sy);
	
	Tool_Brush brush;
	
	UsingBegin(Benchmark bm2("View_BrushPreview: redraw: brush setup"));
	{
		if (settings->patternId == 0)
			brush.Setup(settings->diameter, settings->hardness, false);
		else
		{
			Brush_Pattern* pattern = app.mApplication->BrushLibrarySet_get()->Find(settings->patternId);
			
			brush.Setup_Pattern(settings->diameter, &pattern->mFilter, pattern->mIsOriented);
		}
	}
	UsingEnd();
	
	UsingBegin(Benchmark bm2("View_BrushPreview: redraw: apply brush"));
	{
		AreaI dirty;
		
		const bool cheap = false;
		//const bool cheap = true;
		
		for (size_t i = 0; i < points.size() / 2; ++i)
		{
			Vec2F location = points[i * 2 + 0];
			Vec2F direction = points[i * 2 + 1];
			
//			LOG_DBG("preview point: %f, %f", location[0], location[1]);
			
			if (cheap)
				brush.ApplyFilter_Cheap(&filter, brush.Filter_get(), location[0], location[1], direction[0], direction[1], dirty);
			else
				brush.ApplyFilter(&filter, brush.Filter_get(), location[0], location[1], direction[0], direction[1], dirty);
		}
	}
	UsingEnd();
	
	filter.Multiply(opacity);
	
	brushPreview->Size_set(sx, sy, false);
	
	if (type == ToolViewType_Brush || type == ToolViewType_Smudge)
	{
		filter.ToMacImage(*brushPreview, color);
	}
	else
	{		
		//RenderCheckerBoard(*brushPreview, app.mApplication->LayerMgr_get()->BackColor1_get(), app.mApplication->LayerMgr_get()->BackColor2_get(), 10);
		
		const int c1 = 170;
		const int c2 = 190;
		
		RenderCheckerBoard(*brushPreview, MacRgba_Make(c1, c1, c1, 255), MacRgba_Make(c2, c2, c2, 255), 10);
		
		for (int y = 0; y < filter.Sy_get(); ++y)
		{
			const float* srcLine = filter.Line_get(y);
			MacRgba* dstLine = brushPreview->Line_get(y);
			
			for (int x = 0; x < filter.Sx_get(); ++x)
			{
				dstLine->rgba[3] = (uint8_t)(dstLine->rgba[3] * (*srcLine));
				
				srcLine++;
				dstLine++;
			}
		}
	}
}

-(void)drawRect:(CGRect)rect 
{
	HandleExceptionObjcBegin();
	
	Benchmark bm("View_BrushPreview: drawRect");
	
	LOG_DBG("View_BrushPreview: drawRect", 0);
	
	[self redraw];
	
	CGContextRef ctx = UIGraphicsGetCurrentContext();
	const float scale = [AppDelegate displayScale];
	
	CGImageRef cgImage = brushPreview->ImageWithAlpha_get();
	const int sx = CGImageGetWidth(cgImage);
	const int sy = CGImageGetHeight(cgImage);
	CGRect r = CGRectMake(0.0f, 0.0f, sx / scale, sy / scale);
	CGContextDrawImage(ctx, r, cgImage);
	
	CGImageRelease(cgImage);
	
	HandleExceptionObjcEnd(false);
}

- (void)dealloc
{
	HandleExceptionObjcBegin();
	
	LOG_DBG("dealloc: View_BrushPreview", 0);
	
	delete brushPreview;
	brushPreview = 0;
	
    [super dealloc];
	
	HandleExceptionObjcEnd(false);
}

@end
