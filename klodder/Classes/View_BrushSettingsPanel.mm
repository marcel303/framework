#import "AppDelegate.h"
#import "Application.h"
#import "Bezier.h"
#import "BrushSettingsLibrary.h"
#import "Calc.h"
#import "ExceptionLoggerObjC.h"
#import "Filter.h"
#import "Log.h"
#import "TravellerCapturer.h"
#import "View_BrushSettingsPanel.h"
#import "View_BrushPreview.h"
#import "View_BrushSettings.h"
#import "View_Gauge.h"
#import "View_ToolSelectMgr.h"

#define PREVIEW_SX 200
#define PREVIEW_SY 80
#define GAUGE_HEIGHT 30.0f

@implementation View_BrushSettingsPanel

-(id)initWithFrame:(CGRect)frame app:(AppDelegate*)_app controller:(View_ToolSelectMgr*)_controller type:(ToolViewType)type
{
	HandleExceptionObjcBegin();
	
//	frame.size.width = 250.0f;
//	frame.size.height = 205.0f;
	
    if ((self = [super initWithFrame:frame])) 
	{
#ifdef DEBUG
		[self setOpaque:TRUE];
		[self setBackgroundColor:[UIColor colorWithRed:0.1f green:0.1f blue:0.1f alpha:0.1f]];
#else
		[self setOpaque:FALSE];
		[self setBackgroundColor:[UIColor clearColor]];
#endif
		[self setUserInteractionEnabled:TRUE];
		[self setMultipleTouchEnabled:FALSE];
		
		app = _app;
		controller = _controller;
		
		float y = 0.0f;
		
		UIImageView* viewPatternSelectBack = [[[UIImageView alloc] initWithImage:[[[UIImage alloc] initWithContentsOfFile:[[NSBundle mainBundle] pathForResource:@"pattern_select_back" ofType:@"png"]] autorelease]] autorelease];
		[self addSubview:viewPatternSelectBack];
		[viewPatternSelectBack setTransform:CGAffineTransformMakeScale(100.0f, 1.0f)];
//		[self sendSubviewToBack:viewPatternSelectBack];
		
		viewPatternSelect = [[[View_BrushSelect alloc] initWithFrame:CGRectMake(0.0f, y, frame.size.width, 0.0f) controller:controller delegate:self] autorelease];
		[self addSubview:viewPatternSelect];
		y += viewPatternSelect.frame.size.height + 25.0f;
		
		viewSettings = [[View_BrushSettings alloc] initWithFrame:CGRectMake(0.0f, y, frame.size.width, 0.0f) controller:controller];
		[self addSubview:viewSettings];
		y += viewSettings.frame.size.height + 10.0f;
		
		viewPreview = [[[View_BrushPreview alloc] initWithFrame:CGRectMake((frame.size.width - PREVIEW_SX) / 2.0f, y, PREVIEW_SX, PREVIEW_SY) app:_app settings:app.brushSettings type:type color:Rgba_Make(1.0f, 0.0f, 0.0f)] autorelease];
		[self addSubview:viewPreview];
		y += viewPreview.frame.size.height + 10.0f;
		
		[self addSubview:CreateLabel(0.0f, y, 80.0f, GAUGE_HEIGHT, @"Spacing:", FONT_MARKER, 18.0f, NSTextAlignmentCenter)];
		spacingGauge = [[[View_Gauge alloc] initWithLocation:CGPointMake(80.0f, y) scale:1.0f height:GAUGE_HEIGHT min:gMinBrushSpacing max:gMaxBrushSpacing value:app.brushSettings->spacing * 100.0f unit:@"%" delegate:self changed:@selector(spacingChanged:) provideText:nil] autorelease];
		[self addSubview:spacingGauge];
		y += spacingGauge.frame.size.height + 10.0f;
		
		frame.size.height = y;
		
		[self setFrame:frame];
    }
	
    return self;
	
	HandleExceptionObjcEnd(false);
	
	return nil;
}

-(void)brushSettingsChanged
{
	[viewPatternSelect brushSettingsChanged];
	[viewSettings brushSettingsChanged];

	[spacingGauge setValueDirect:app.brushSettings->spacing * 100.0f];
	
	[viewPreview setNeedsDisplay];
}

-(void)spacingChanged:(NSNumber*)value;
{
	HandleExceptionObjcBegin();
	
	const float spacing = [value intValue] / 100.0f;
	
	app.brushSettings->spacing = spacing;
	
	[controller brushSettingsChanged];
	
	HandleExceptionObjcEnd(false);
}

// ====================
// BrushSelectDelegate implementation
// ====================
-(std::vector<Brush_Pattern*>)getPatternList
{
	return app.mApplication->BrushLibrarySet_get()->BrushList_get();
}

-(void)handleBrushPatternSelect:(int)patternId
{
	[controller brushPatternChanged:patternId];
}

-(void)handleBrushSoftSelect
{
	[controller brushPatternChanged:0];
}

//

-(void)dealloc
{
	HandleExceptionObjcBegin();
	
	LOG_DBG("dealloc: View_BrushSettingsPanel", 0);
	
//	[viewPattern release];
	[viewSettings release];
	
    [super dealloc];
	
	HandleExceptionObjcEnd(false);
}

@end
