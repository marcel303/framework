#import "AppDelegate.h"
#import "BrushSettingsLibrary.h"
#import "ExceptionLoggerObjC.h"
#import "Log.h"
#import "Settings.h"
#import "View_BrushSettingsPanel.h"
#import "View_CheckBox.h"
#import "View_Gauge.h"
#import "View_ToolSelect_NavBar.h"
#import "View_MenuSelection.h"
#import "View_ToolSelectMgr.h"
#import "View_ToolSettings_Smudge.h"

#define FONT @"Marker Felt"

#define IMG_BACK [UIImage imageNamed:@IMG("back_pattern")]

@implementation View_ToolSettings_Smudge

-(id)initWithFrame:(CGRect)frame andApp:(AppDelegate*)_app controller:(View_ToolSelectMgr*)_controller
{
	HandleExceptionObjcBegin();
	
	LOG_DBG("View_ToolSettings_Smudge init", 0);
	
    if ((self = [super initWithFrame:frame])) 
	{
		app = _app;
		controller = _controller;
		
		viewToolType = [[[View_ToolType alloc] initWithFrame:CGRectMake(0.0f, 0.0f, 320.0f, 50.0f) toolType:ToolViewType_Smudge delegate:controller] autorelease];
		[self addSubview:viewToolType];
		
		float x = 0.0f;
		float y = 110.0f;
		
		viewBrushSettings = [[[View_BrushSettingsPanel alloc] initWithFrame:CGRectMake(x, y, 320.0f, 190.0f) app:app controller:controller type:ToolViewType_Smudge] autorelease];
		[self addSubview:viewBrushSettings];
		y += viewBrushSettings.frame.size.height;
		
		[self addSubview:CreateLabel(x, y, 80.0f, 30.0f, @"Strength:", FONT_MARKER, 18.0f, NSTextAlignmentCenter)];
		viewStrengthGauge = [[[View_Gauge alloc] initWithLocation:CGPointMake(x + 80.0f, y) scale:1.0f height:30.0f min:0 max:100 value:app.brushSettings->strength * 100.0f unit:@"%" delegate:self changed:@selector(strengthChanged:) provideText:nil] autorelease];
		[self addSubview:viewStrengthGauge];
		y += viewStrengthGauge.frame.size.height;
		
		[self addSubview:[[[View_MenuSelection alloc] initWithIndex:1 frame:frame app:app controller:controller] autorelease]];
		[self addSubview:[[[View_ToolSelect_NavBar alloc] initWithFrame:self.bounds name:@"Smudge" controller:controller] autorelease]];
    }
	
    return self;
	
	HandleExceptionObjcEnd(false);
	
	return nil;
}

-(void)load
{
	LOG_DBG("View_ToolSettings_Smudge: load", 0);
	
	uint32_t patternId = [View_ToolSettings_Smudge lastPatternId];
	
	app.brushSettings->patternId = patternId;
}

-(void)save
{
	LOG_DBG("View_ToolSettings_Smudge: save", 0);
	
	gSettings.SetUInt32("tool.smudge.pattern_id", app.brushSettings->patternId);
}

-(void)brushSettingsChanged
{
	[viewBrushSettings brushSettingsChanged];
	
	[viewStrengthGauge setValueDirect:app.brushSettings->strength * 100.0f];
}

-(void)strengthChanged:(NSNumber*)value
{
	app.brushSettings->strength = [value floatValue] / 100.0f;
	
	[controller brushSettingsChanged];
}

+(uint32_t)lastPatternId
{
	return gSettings.GetUInt32("tool.smudge.pattern_id", 0);
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
	
	LOG_DBG("dealloc: View_ToolSettings_Smudge", 0);
	
    [super dealloc];
	
	HandleExceptionObjcEnd(false);
}

@end
