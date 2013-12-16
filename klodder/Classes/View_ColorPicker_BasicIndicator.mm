#import "ExceptionLoggerObjC.h"
#import "Log.h"
#import "View_ColorPicker_BasicIndicator.h"

#import "AppDelegate.h" // IMG()

#define IMG_CIRCLE [UIImage imageNamed:@IMG("indicator_circle")]

@implementation View_ColorPicker_BasicIndicator

-(id)init 
{
	HandleExceptionObjcBegin();
	
	UIImage* circle = IMG_CIRCLE;
	
	CGRect frame = CGRectMake(0.0f, 0.0f, circle.size.width, circle.size.height);
	
    if ((self = [super initWithFrame:frame])) 
	{
		[self setOpaque:FALSE];
		[self setBackgroundColor:[UIColor clearColor]];
    }
	
    return self;
	
	HandleExceptionObjcEnd(false);
	
	return nil;
}

-(void)drawRect:(CGRect)rect 
{
	HandleExceptionObjcBegin();
	
	[IMG_CIRCLE drawAtPoint:CGPointMake(0.0f, 0.0f)];

	HandleExceptionObjcEnd(false);
}

-(void)dealloc
{
	HandleExceptionObjcBegin();
	
	LOG_DBG("dealloc: View_ColorPicker_BasicIndicator", 0);
	
    [super dealloc];
	
	HandleExceptionObjcEnd(false);
}

@end
