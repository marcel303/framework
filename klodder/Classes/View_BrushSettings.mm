#import "BrushSettingsLibrary.h"
#import "ExceptionLoggerObjC.h"
#import "Log.h"
#import "View_BrushSettings.h"
#import "View_Gauge.h"
#import "View_ToolSelectMgr.h"

@implementation View_BrushSettings

#define GAUGE_HEIGHT 30.0f

- (id)initWithFrame:(CGRect)frame controller:(View_ToolSelectMgr*)_controller
{
	HandleExceptionObjcBegin();
	
    if ((self = [super initWithFrame:frame])) 
	{
		[self setOpaque:FALSE];
		[self setBackgroundColor:[UIColor clearColor]];
		[self setUserInteractionEnabled:TRUE];
		[self setMultipleTouchEnabled:FALSE];
		
		controller = _controller;
		
		float y = 0.0f;
		
		[self addSubview:CreateLabel(0.0f, y, 80.0f, GAUGE_HEIGHT, @"Size:", FONT_MARKER, 18.0f, NSTextAlignmentCenter)];
		sizeGauge = [[[View_Gauge alloc] initWithLocation:CGPointMake(80.0f, y) scale:1.0f height:GAUGE_HEIGHT min:1 max:gMaxBrushRadius value:controller.brushSettings->diameter unit:@"px" delegate:self changed:@selector(sizeChanged:) provideText:nil] autorelease];
		[self addSubview:sizeGauge];
		y += sizeGauge.frame.size.height + 5.0f;
		
		[self addSubview:CreateLabel(0.0f, y, 80.0f, GAUGE_HEIGHT, @"Hardness:", FONT_MARKER, 18.0f, NSTextAlignmentCenter)];
		hardnessGauge = [[[View_Gauge alloc] initWithLocation:CGPointMake(80.0f, y) scale:1.0f height:GAUGE_HEIGHT min:0 max:100 value:controller.brushSettings->hardness * 100.0f unit:@"%"delegate:self changed:@selector(hardnessChanged:) provideText:nil] autorelease];
		[self addSubview:hardnessGauge];
		y += hardnessGauge.frame.size.height + 5.0f;
		
		frame.size.height = y;
		
		[self setFrame:frame];
    }
	
    return self;
	
	HandleExceptionObjcEnd(false);
	
	return nil;
}

-(void)sizeChanged:(NSNumber*)value;
{
	HandleExceptionObjcBegin();

	const int diameter = [value intValue];
	
	controller.brushSettings->diameter = diameter;
	
	[controller brushSettingsChanged];
	
	HandleExceptionObjcEnd(false);
}

-(void)hardnessChanged:(NSNumber*)value
{
	HandleExceptionObjcBegin();

	const float hardness = [value intValue] / 100.0f;
	
	controller.brushSettings->hardness = hardness;
	
	[controller brushSettingsChanged];
	
	HandleExceptionObjcEnd(false);
}

-(void)brushSettingsChanged
{
	HandleExceptionObjcBegin();
	
	[sizeGauge setValueDirect:controller.brushSettings->diameter];
	[hardnessGauge setValueDirect:controller.brushSettings->hardness * 100.0f];
	[hardnessGauge setEnabled:controller.brushSettings->patternId == 0];

	HandleExceptionObjcEnd(false);
}

/*-(NSString*)gaugeSizeText:(NSNumber*)gaugeValue
{
	return [NSString stringWithFormat:@"%dpx", [gaugeValue intValue] * 2 + 1];
}*/

- (void)dealloc
{
	HandleExceptionObjcBegin();
	
	LOG_DBG("dealloc: View_BrushSettings_Soft", 0);
	
    [super dealloc];
	
	HandleExceptionObjcEnd(false);
}

@end
