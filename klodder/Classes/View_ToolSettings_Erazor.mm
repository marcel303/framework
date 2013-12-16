#import "AppDelegate.h"
#import "BrushSettingsLibrary.h"
#import "ExceptionLoggerObjC.h"
#import "Settings.h"
#import "View_BrushSettingsPanel.h"
#import "View_CheckBox.h"
#import "View_Gauge.h"
#import "View_ImageButton.h"
#import "View_MenuSelection.h"
#import "View_ToolSelect_NavBar.h"
#import "View_ToolSelectMgr.h"
#import "View_ToolSettings_Erazor.h"

#define IMG_BACK [UIImage imageNamed:@IMG("back_pattern")]

@implementation View_ToolSettings_Eraser

-(id)initWithFrame:(CGRect)frame andApp:(AppDelegate*)_app controller:(View_ToolSelectMgr*)_controller
{
	HandleExceptionObjcBegin();
	
	LOG_DBG("View_ToolSettings_Eraser init", 0);
	
    if ((self = [super initWithFrame:frame])) 
	{
		app = _app;
		controller = _controller;
		
		vwToolType = [[[View_ToolType alloc] initWithFrame:CGRectMake(0.0f, 0.0f, 320.0f, 50.0f) toolType:ToolViewType_Eraser delegate:controller] autorelease];
		[self addSubview:vwToolType];
		
		float x = 0.0f;
		float y = 110.0f;
		
		vwBrushSettingsPanel = [[[View_BrushSettingsPanel alloc] initWithFrame:CGRectMake(x, y, 320.0f, 190.0f) app:app controller:controller type:ToolViewType_Eraser] autorelease];
		[self addSubview:vwBrushSettingsPanel];
		y += vwBrushSettingsPanel.frame.size.height;
		
		[self addSubview:CreateLabel(x, y, 80.0f, 30.0f, @"Strength:", FONT_MARKER, 18.0f, UITextAlignmentCenter)];
		strengthGauge = [[[View_Gauge alloc] initWithLocation:CGPointMake(80.0f, y) scale:1.0f height:30.0f min:0 max:100 value:app.colorPickerState->Opacity_get() * 100.0f unit:@"%" delegate:self changed:@selector(opacityChanged:) provideText:nil] autorelease];
		[self addSubview:strengthGauge];
		y += strengthGauge.frame.size.height;
		
		[self addSubview:[[[View_MenuSelection alloc] initWithIndex:1 frame:frame app:app controller:controller] autorelease]];
		[self addSubview:[[[View_ToolSelect_NavBar alloc] initWithFrame:self.bounds name:@"Eraser" controller:controller] autorelease]];
    }
	
    return self;
	
	HandleExceptionObjcEnd(false);
	
	return nil;
}

-(void)load
{
	LOG_DBG("View_ToolSettings_Eraser: load", 0);
	
	uint32_t patternId = [View_ToolSettings_Eraser lastPatternId];
	
	app.brushSettings->patternId = patternId;
}

-(void)save
{
	LOG_DBG("View_ToolSettings_Eraser: save", 0);
	
	gSettings.SetUInt32("tool.eraser.pattern_id", app.brushSettings->patternId);
}

-(void)brushSettingsChanged
{
	[vwBrushSettingsPanel brushSettingsChanged];
}

-(void)opacityChanged:(NSNumber*)value;
{
	HandleExceptionObjcBegin();
	
	app.colorPickerState->Opacity_set([value floatValue] / 100.0f);
	
	[controller brushSettingsChanged];
	
	HandleExceptionObjcEnd(false);
}

+(uint32_t)lastPatternId
{
	return gSettings.GetUInt32("tool.eraser.pattern_id", 0);
}

-(void)drawRect:(CGRect)rect
{
	HandleExceptionObjcBegin();
	
	// draw background
	
	[IMG_BACK drawAsPatternInRect:self.frame];
	
	HandleExceptionObjcEnd(false);
}

-(void)dealloc
{
	HandleExceptionObjcBegin();
	
	LOG_DBG("dealloc: View_ToolSettings_Eraser", 0);
	
    [super dealloc];
	
	HandleExceptionObjcEnd(false);
}

@end
