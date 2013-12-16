#import "Calc.h"
#import "ExceptionLoggerObjC.h"
#import "Log.h"
#import "View_Gauge.h"
#import "View_Gauge_ActiveRegion.h"
#import "View_Gauge_Button.h"
#import "ViewBase.h"

#import "AppDelegate.h" // IMG()

#define TEXT_SX 30.0f

@implementation View_Gauge

@synthesize value;
@synthesize enabled;

-(id)initWithLocation:(CGPoint)location scale:(float)scale height:(float)height min:(int)_min max:(int)_max value:(int)_value unit:(NSString*)_unitText delegate:(id)_delegate changed:(SEL)_changed provideText:(SEL)_provideText
{
	HandleExceptionObjcBegin();
	
	float buttonSize = height;
	
	UIImage* back = IMG_GAUGE_BACK;
	
	float sx = [back size].width * scale + buttonSize * 2.0f + TEXT_SX;
	
	CGRect frame = CGRectMake(location.x, location.y, sx, height);
	
    if ((self = [super initWithFrame:frame])) 
	{
		[self setOpaque:FALSE];
		[self setBackgroundColor:[UIColor clearColor]];
		[self setMultipleTouchEnabled:FALSE];
		
		min = _min;
		max = _max;
		value = _value;
		enabled = true;
		unitText = _unitText;
		
		delegate = _delegate;
		changed = _changed;
		provideText = _provideText;
		
		float x = 0.0f;
		less = [[[View_Gauge_Button alloc] initWithFrame:CGRectMake(x, 0.0f, buttonSize, buttonSize) gauge:self direction:-1] autorelease];
		[self addSubview:less];	
		x += less.frame.size.width;
		activeRegion = [[[View_Gauge_ActiveRegion alloc] initWithFrame:CGRectMake(x, 0.0f, [back size].width, frame.size.height) gauge:self] autorelease];
		[self addSubview:activeRegion];
		x += activeRegion.frame.size.width;
		more = [[[View_Gauge_Button alloc] initWithFrame:CGRectMake(x, 0.0f, buttonSize, buttonSize) gauge:self direction:+1] autorelease];
		[self addSubview:more];
		x += more.frame.size.width;
		
		label = CreateLabel(x, 0.0f, TEXT_SX, frame.size.height, @"", FONT_TYPWRITER, 10.0f, UITextAlignmentLeft);
		[self addSubview:label];
		
		[self setValueDirect:value];
    }
	
    return self;
	
	HandleExceptionObjcEnd(false);
	
	return nil;
}

-(void)setValue:(int)_value
{
	if (_value == value)
		return;
	
	[self setValueDirect:_value];
	
	[delegate performSelector:changed withObject:[NSNumber numberWithInt:value]];
}

-(void)setValueDirect:(int)_value
{
	_value = Calc::Mid(_value, min, max);
	
	value = _value;
	
	NSString* text;
	
	if (provideText != nil)
		text = [delegate performSelector:provideText withObject:[NSNumber numberWithInt:value]];
	else
		text = [NSString stringWithFormat:@"%d%@", value, unitText];
		
	[label setText:text];
	
	[activeRegion setNeedsDisplay];
}

-(float)fill
{
	return (value - min) / (float)(max - min);
}

-(int)fillToValue:(float)fill
{
	int temp = (int)Calc::RoundNearest(min + (max - min) * fill);
	
	return Calc::Mid(temp, min, max);
}

-(void)setEnabled:(bool)_enabled
{
	enabled = _enabled;
	
	[less setNeedsDisplay];
	[more setNeedsDisplay];
	[activeRegion setNeedsDisplay];
}

-(void)dealloc
{
	HandleExceptionObjcBegin();
	
	LOG_DBG("dealloc: View_Gauge", 0);
	
    [super dealloc];
	
	HandleExceptionObjcEnd(false);
}

@end
