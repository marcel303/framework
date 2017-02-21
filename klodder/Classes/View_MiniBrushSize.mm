#import "AppDelegate.h"
#import "Application.h"
#import "BrushSettingsLibrary.h"
#import "ExceptionLoggerObjC.h"
#import "Log.h"
#import "View_BrushPreview.h"
#import "View_MiniBrushSize.h"
#import "View_MiniToolSelect.h"
#import "View_ToolSelectMgr.h"
#import "View_ToolSettings_Brush.h"
#import "View_ToolSettings_Erazor.h"
#import "View_ToolSettings_Smudge.h"

@implementation View_MiniBrushSize

-(id)initWithFrame:(CGRect)frame app:(AppDelegate*)_app parent:(View_MiniBrushSizeContainer*)_parent
{
	HandleExceptionObjcBegin();
	
    if ((self = [super initWithFrame:frame])) 
	{
		[self setOpaque:FALSE];
		[self setBackgroundColor:[UIColor clearColor]];
		[self setClearsContextBeforeDrawing:TRUE];
		
		//
		
		app = _app;
		parent = _parent;
		
		//
		
		preview = [[[View_BrushPreview alloc] initWithFrame:CGRectMake(25.0f, 15.0f, 200.0f, 70.0f) app:_app settings:_app.brushSettings type:ToolViewType_Brush color:Rgba_Make(1.0f, 1.0f, 1.0f, 1.0f)] autorelease];
		[self addSubview:preview];
		
		slider = [[[UISlider alloc] initWithFrame:CGRectMake(25.0, 100.0f, 200.0f, 20.0f)] autorelease];
		[slider setMinimumValue:1.0f];
		[slider setMaximumValue:MAX_BRUSH_RADIUS_UI];
		[slider addTarget:self action:@selector(handleSliderChanged) forControlEvents:UIControlEventTouchUpInside|UIControlEventValueChanged];
		[slider addTarget:self action:@selector(handleSliderUp) forControlEvents:UIControlEventTouchUpInside|UIControlEventTouchUpOutside];
		[self addSubview:slider];
		
		toolSelect = [[[View_MiniToolSelect alloc] initWithFrame:CGRectMake(0.0f, 0.0f, 200.0f, 60.0f) delegate:self action:@selector(handleToolSelect:)] autorelease];
		[self addSubview:toolSelect];
		
		[self updateUi];
    }
	
    return self;
	
	HandleExceptionObjcEnd(false);
	
	return nil;
}

-(void)show
{
	[app loadCurrentBrushSettings];
		
	[self updateUi];
	
//	[parent show];
}

-(void)hide
{
	[app applyBrushSettings:app.mApplication->ToolType_get()];
	
	[app saveBrushSettings];
}

-(void)updateUi
{
	[slider setValue:app.brushSettings->diameter];
	
	[preview setNeedsDisplay];
}

-(void)handleSliderUp
{
	HandleExceptionObjcBegin();
	
	[parent hide];
	
	HandleExceptionObjcEnd(false);
}

-(void)handleSliderChanged
{
	HandleExceptionObjcBegin();
	
	int diameter = slider.value;
	
	app.brushSettings->diameter = diameter;
	
	[self updateUi];
	
	HandleExceptionObjcEnd(false);
}

-(void)handleToolSelect:(NSNumber*)_type
{
	HandleExceptionObjcBegin();
	
	[app saveBrushSettings];
	
	ToolViewType type = (ToolViewType)[_type intValue];
	ToolType toolType = ToolType_Undefined;
	
	uint32_t patternId = 0;
	
	switch (type)
	{
		case ToolViewType_Brush:
			patternId = [View_ToolSettings_Brush lastPatternId];
			toolType = patternId ? ToolType_PatternBrush : ToolType_SoftBrush;
			break;
		case ToolViewType_Smudge:
			patternId = [View_ToolSettings_Smudge lastPatternId];
			toolType = patternId ? ToolType_PatternSmudge : ToolType_SoftSmudge;
			break;
		case ToolViewType_Eraser:
			patternId = [View_ToolSettings_Eraser lastPatternId];
			toolType = patternId ? ToolType_PatternEraser : ToolType_SoftEraser;
			break;
		case ToolViewType_Undefined:
			break;
	}
	
	if (toolType == ToolType_Undefined)
	{
		toolType = ToolType_SoftBrush;
		patternId = 0;
	}
	
	BrushSettingsLibrary library;
	
	BrushSettings settings = library.Load(toolType, patternId, 0);
	
	app.brushSettings = &settings;
	
	[app applyBrushSettings:toolType];
	
	[self updateUi];
	
	HandleExceptionObjcEnd(false);
}

-(void)touchesBegan:(NSSet *)touches withEvent:(UIEvent *)event
{
	HandleExceptionObjcBegin();
	
	HandleExceptionObjcEnd(false);
}

-(void)touchesEnded:(NSSet *)touches withEvent:(UIEvent *)event
{
	HandleExceptionObjcBegin();
	
	[parent hide];
	
	HandleExceptionObjcEnd(false);
}

static void DrawRoundedRect(CGContextRef context, float x, float y, float sx, float sy, float radius, UIColor* fillColor, UIColor* strokeColor)
{
	float stroke = 1.5f;
	
    CGContextSetLineWidth(context, stroke);
    CGContextSetFillColorWithColor(context, fillColor.CGColor);
    CGContextSetStrokeColorWithColor(context, strokeColor.CGColor);
//	CGContextSetAlpha(context, 0.5f);
    
	// clamp radius to size
	
    if (radius > sx * 0.5f)
        radius = sx * 0.5f;
    if (radius > sy * 0.5f)
        radius = sy * 0.5f;
    
    CGFloat minx = x + stroke;
    CGFloat midx = x + sx * 0.5f;
    CGFloat maxx = x + sx - stroke;
    CGFloat miny = y + stroke;
    CGFloat midy = y + sy * 0.5f;
    CGFloat maxy = y + sy - stroke;
	
    CGContextMoveToPoint(context, minx, midy);
    CGContextAddArcToPoint(context, minx, miny, midx, miny, radius);
    CGContextAddArcToPoint(context, maxx, miny, maxx, midy, radius);
    CGContextAddArcToPoint(context, maxx, maxy, midx, maxy, radius);
    CGContextAddArcToPoint(context, minx, maxy, minx, midy, radius);
    CGContextClosePath(context);
	
    CGContextDrawPath(context, kCGPathFillStroke);
}

-(void)drawRect:(CGRect)rect 
{
	HandleExceptionObjcBegin();
	
	LOG_DBG("View_MiniBrushSize: drawRect", 0);
	
	CGContextRef context = UIGraphicsGetCurrentContext();
	
	DrawRoundedRect(context, 0.0f, 0.0f, self.frame.size.width, self.frame.size.height, 20.0f, [UIColor colorWithRed:0.0f green:0.0f blue:0.0f alpha:0.7f], [UIColor whiteColor]);
	
	HandleExceptionObjcEnd(false);
}

-(void)dealloc 
{
	HandleExceptionObjcBegin();
	
    [super dealloc];
	
	HandleExceptionObjcEnd(false);
}

@end
